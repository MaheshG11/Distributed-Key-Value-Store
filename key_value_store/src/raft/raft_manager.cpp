#include "raft_manager.h"

raftManager::raftManager(int32_t election_timeout_low_,
                            int32_t election_timeout_high_,
                            int32_t heartbeat_timeout_,
                            std::string &this_ip_port_,
                            int32_t max_retries_,
                            std::string master_ip_port_,
                            STATE state_,
                            std::string cluster_key_)
                        {
    state=state_;
    cluster_key=cluster_key_;
    election_timeout_low = std::chrono::milliseconds(election_timeout_low_);
    election_timeout_high = std::chrono::milliseconds(election_timeout_high_);
    election_timeout = std::chrono::milliseconds(
                            get_random(election_timeout_low_,
                                election_timeout_high_));
    heartbeat_timeout = std::chrono::milliseconds(heartbeat_timeout_);
    max_retries = max_retries_;
    last_contact = std::chrono::system_clock::now();
    last_voted = last_contact;
    this_ip_port=this_ip_port_;
    if(state_==MASTER)master_ip_port_=this_ip_port_;
    update_master(master_ip_port_,0);
    if(state_==FOLLOWER){
        grpc::ClientContext context;
        ::member_response response;
        ::member_request request;
        request.set_cluster_key(cluster_key);
        request.set_to_drop(false);
        request.set_ip_port(this_ip_port_);
        grpc::Status status=master_stub->
        update_cluster_member(&context,request,&response);
        add_node_to_cluster(master_ip_port);
        if(!status.ok()){
            std::cerr<<"Unable to reach master\n"; 
            std::cerr<<"either master is not running OR master ip-port is incorrect\n";
            std::cerr<<"master ip-port you entered:"<<master_ip_port_<<'\n';
            exit(EXIT_FAILURE); 
        }
        else if(!response.success()){
            std::cerr<<"Cluster Key is incorrect\n";
            std::cerr<<"Cluster Key you entered:"<<cluster_key_<<'\n';
            exit(EXIT_FAILURE); 
        }
        else if(response.success()){
            std::cout<<"connected to master on :"<<master_ip_port_<<'\n';
            start_heartbeat_sensing();
        }
        
    }
    update_last_contact();
    update_last_voted();

}


STUB::STUB(std::string &ip_port){
    s1=key_value_store_rpc::NewStub(grpc::CreateChannel(
            ip_port, grpc::InsecureChannelCredentials()
    ));
    s2=raft::NewStub(grpc::CreateChannel(
            ip_port, grpc::InsecureChannelCredentials()
    ));
}




raftManager::~raftManager(){
    stop_heartbeat_sensing();
    if(heartbeat_sensing_thread.joinable()){
        heartbeat_sensing_thread.join();
    }
}
void raftManager::add_node_to_cluster(std::string ip_port){
    STUB s(ip_port);
    mpp.insert(std::move(std::make_pair(ip_port,std::move(s))));
    std::cout<<"number of members I am aware of :"<<mpp.size()<<'\n';
}

// bool raftManager::broadcast_log_entry(T &request);
// The definition of this is in header file since it is a template


bool raftManager::broadcast_commit(int64_t entry_id, bool commit){
    ::commit_request request;
    request.set_entry_id(entry_id);
    request.set_commit(commit);
    std::unique_lock<std::mutex> master_info_lock(master_info_mutex);
    request.set_term(term_id);
    master_info_lock.unlock();
    int32_t sz=mpp.size();
    grpc::ClientContext context;
    std::vector<::commit_response> response(sz);
    std::vector<grpc::Status> status(sz);
    std::vector<std::future<void>> futures(sz);
    int cnt=0,j=0;
    for(auto &i : mpp){
        int32_t j_copy=j++;
        futures[j_copy]=std::async(
            std::launch::async,
            [&,j_copy](){
                return retry([&](grpc::ClientContext* ctx, const commit_request& req, commit_response* res) {
                    return i.second.s2->commit_log_entry(ctx, req, res);
                },
                        heartbeat_timeout,(status[j_copy]),
                        request, &(response[j_copy])
                    );
            }
        );
    }
    for(int32_t i=0;i<j;i++){
        
            futures[i].get();
            if(!status[i].ok() || !response[i].success())
            {
                continue;
            } 
            cnt++;
    }
    if(cnt>=(get_nodes_cnt()/2)) return true;
    return false;    
}

bool raftManager::broadcast_member_update(::member_request request){
    int32_t sz=mpp.size();
    grpc::ClientContext context;
    std::vector<::member_response> response(sz);
    std::vector<grpc::Status> status(sz);
    std::vector<std::future<void>> futures(sz);

    int cnt=0,j=0;
    for(auto &i : mpp){
        int32_t j_copy=j++;
        futures[j_copy]=std::async(
            std::launch::async,
                [&,j_copy](){
                    return retry([&](grpc::ClientContext* ctx, const member_request& req, member_response* res) {
                        return i.second.s2->update_cluster_member(ctx, req, res);
                },heartbeat_timeout,(status[j_copy]),
                            request, &(response[j_copy])
                        );
                }
        );

    }

    for(int32_t i=0;i<j;i++){
        try{
            futures[i].get();
            if(!status[i].ok() || !response[i].success())
            {
                continue;
            } 
            cnt++;
        } catch (const std::exception& e) {
            continue;
        }
    }

    if(cnt>=(get_nodes_cnt()/2)) {
        share_cluster_info_with(request.ip_port(),request.cluster_key());
        return true;
    
    }
    return false;
}

void raftManager::start_heartbeat_sensing(){
    stop_heartbeat_sensing();
    std::unique_lock<std::mutex> heartbeat_lock(heartbeat_mutex);
    std::cout<<"waiting\n";
    heartbeat_cv.wait(heartbeat_lock,[this](){
        return is_running==false;
    });
    std::cout<<"not waiting\n";
    if(heartbeat_sensing_thread.joinable())heartbeat_sensing_thread.join();
    heartbeat_sensing_thread = std::thread(&raftManager::start,this);

}

void raftManager::start(){
    std::unique_lock<std::mutex> last_contact_lock(last_contact_mutex,std::defer_lock),
                                    heartbeat_lock(heartbeat_mutex),
                                    master_info_lock(master_info_mutex,std::defer_lock);

    std::cout<<"started heartbeat thread"<<'\n'; 
    is_running = true;
    run_heartbeat_sensing = true;
    heartbeat_lock.unlock();
    bool is_Active=true;
    while(run_heartbeat_sensing){
        wait_for(heartbeat_timeout);
        is_Active=false;
        if(get_state()==FOLLOWER){
            last_contact_lock.lock();
            auto diff=std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()-last_contact
            );
            last_contact_lock.unlock();
            if(diff<=heartbeat_timeout){
                is_Active=true;
                continue;
            }
            is_Active=send_heartbeat();
            if(is_Active)continue;
        }
        break;
    }
    heartbeat_lock.lock();
    is_running = false;
    heartbeat_lock.unlock();
    heartbeat_cv.notify_one();
    if(!is_Active){
        start_voting();
    }
}
bool raftManager::send_heartbeat(){

    ::heart_request request;
    ::beats_response response;
    grpc::ClientContext context;
    std::unique_lock<std::mutex>master_info_lock(master_info_mutex);
    request.set_term(term_id);
    grpc::Status status;
    if(master_stub!=nullptr){
        master_info_lock.unlock();
        retry([&](grpc::ClientContext* ctx, const heart_request& req, beats_response* res) {
            return master_stub->heart_beat(ctx, req, res);
        },heartbeat_timeout,status,
            request, &response
        );
        if(status.ok()){
            if(response.is_master())
            {
                update_last_contact();
                std::cout<<"heard from master\n";
                return true;
            } else if(response.term()>term_id
                && response.master_ip_port() != "")
            {
                
                std::cout<<"new master found\n";
                update_master(response.master_ip_port(),response.term());
                return true;
            }
        }
    }
    else master_info_lock.unlock();
    return false;
}
void raftManager::start_voting(){
    std::cout<<"invoking election function\n";
    std::unique_lock<std::mutex> lock(last_voted_mutex,std::defer_lock);
    int32_t term_id_=get_term_id();
    std::cout<<"going to start election\n";

    while( (!send_heartbeat() )&&((get_last_contact()+heartbeat_timeout)<std::chrono::system_clock::now())){
        std::cout<<"inside loop\n";
        wait_for(election_timeout);
        lock.lock();
        std::cout<<"acquired lock\n";
        auto diff=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now()-last_voted
        );
        lock.unlock();
        if(diff<=election_timeout_high){
            diff=election_timeout_high-diff;
            std::cout<<"continuing\n";
            wait_for(diff);
            continue;
        }else if(send_heartbeat()){
            std::cout<<"found a master\n";
            break;
        }
        change_state_to(CANDIDATE,"",term_id_+1);
        std::cout<<"election started\n";
        update_last_voted();
        int32_t sz=mpp.size();
        ::vote_request request;
        request.set_ip_port(this_ip_port);
        request.set_term(term_id_+1);
        std::vector<::vote_response> response(sz);
        std::vector<grpc::Status> status(sz);
        std::vector<std::future<void>> futures(sz);
        int cnt=1,j=0;
        std::cout<<"sending out vote requests\n";
        for(auto &i : mpp){
                int32_t j_copy=j++;
                futures[j_copy]=std::async(
                    std::launch::async,
                    [&,j_copy](){
                        return retry([&](grpc::ClientContext* ctx, const vote_request& req, vote_response* res) {
                            return i.second.s2->vote_rpc(ctx, req, res);
                        },heartbeat_timeout,(status[j_copy]),
                                request, &(response[j_copy])
                            );
                    }
                );
        }
        std::cout<<"asked for votes\n"; 
        for(int32_t i=0;i<j;i++){
            try{
                futures[i].get();
                if(!status[i].ok())
                {
                    continue;
                }
                if(response[i].vote()) cnt++;
            } catch (const std::exception& e) {
                continue;
            }
        }
        std::cout<<"got "<<cnt<<" votes\n";
        if(cnt>(get_nodes_cnt()/2) && (get_last_contact()+heartbeat_timeout)<std::chrono::system_clock::now()) {
            change_state_to(MASTER,this_ip_port,term_id_+1);
            broadcast_new_master();
            stop_heartbeat_sensing();
            break;
        }
    }
    std::cout<<"returned\n";
}

// assuming broadcast is guaranteed
void raftManager::broadcast_new_master(){
    std::cout<<"broadcasting that I am a master\n";
    ::master_change_request request;
    request.set_ip_port(this_ip_port);
    request.set_term(get_term_id());
    ::master_change_response response;
    std::vector<std::future<void>> futures(mpp.size());
    grpc::Status status;
    int cnt=0,j=0;
    for(auto &i : mpp){
        int32_t j_copy=j++;
        futures[j_copy]=std::async
            (std::launch::async,
                [&,j_copy](){
                    return retry([&](grpc::ClientContext* ctx, const master_change_request& req, master_change_response* res) {
                        return i.second.s2->new_master(ctx, req, res);
                    },heartbeat_timeout,(status),
                            request, &(response)
                        );
                }
            );

    }
    for(auto &i:futures)i.get();
}


void raftManager::share_cluster_info_with(std::string ip_port,std::string cluster_key_){
    int32_t sz=mpp.size();
    std::vector<::member_request> request(sz);
    std::vector<::member_response> response(sz);
    int32_t j=0;
    std::vector<grpc::ClientContext> context(sz);
    auto it = mpp.find(ip_port);
    if(it==mpp.end()){
        return;
    }
    std::vector<grpc::Status> status(mpp.size());
    std::vector<std::future<void>> futures(mpp.size());
    for(auto &i : mpp){
        request[j].set_cluster_key(cluster_key);
        request[j].set_to_drop(false);
        request[j].set_ip_port(i.first);
        int j_copy=j;

        futures[j_copy]=std::async(
            std::launch::async,
                [&,j_copy](){
                    return retry([&](grpc::ClientContext* ctx, const member_request& req, member_response* res) {
                        return (it->second).s2->update_cluster_member(ctx, req, res);
                    },election_timeout_high,(status[j_copy]),
                            request[j_copy], &(response[j_copy])
                        );
                }
        );
        j++;
    }
    
    for(int32_t i=0;i<j;i++){
        futures[i].get();
        if(!status[i].ok())
        {
            continue;
        }
    }
}

int32_t raftManager::get_random(int32_t min, int32_t max) {
    if(max<min)throw std::runtime_error("high is less then low");
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

std::chrono::time_point<std::chrono::system_clock> raftManager::get_last_contact(){
    std::unique_lock<std::mutex> last_contact_lock(last_contact_mutex);
    return last_contact;

}

bool raftManager::can_vote(int32_t term_id_){
    std::unique_lock<std::mutex> lock(last_voted_mutex),lock_(master_info_mutex);
    auto diff=std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()-last_voted
            );
    return (diff>election_timeout_high) && (term_id_>term_id);
}