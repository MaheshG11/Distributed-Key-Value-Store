#include "raft_manager.h"
#include <thread>  



STUB::STUB(std::string &ip_port){
    s1=key_value_store_rpc::NewStub(grpc::CreateChannel(
            ip_port, grpc::InsecureChannelCredentials()
    ));
    s2=raft::NewStub(grpc::CreateChannel(
            ip_port, grpc::InsecureChannelCredentials()
    ));
}

void raftManager::add_node_to_cluster(std::string ip_port){
    STUB s(ip_port);
    mpp.insert(std::move(std::make_pair(ip_port,std::move(s))));

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

    grpc::ClientContext context;
    std::vector<::commit_response> response(mpp.size());
    std::vector<std::future<grpc::Status>> status;
    int cnt=0,j=0;
    for(auto &i : mpp){
        try {
            int32_t j_copy=j++;
            status.push_back(std::async
                (std::launch::async,
                    [&,j_copy](){
                        return retry(
                            [&](grpc::ClientContext* ctx, const ::commit_request& req, ::commit_response* res) {
                                return i.second.s2->commit_log_entry(ctx, req, res);
                            },
                            heartbeat_timeout,
                            &context, request, &(response[j_copy])
                        );
                    }
                )
            );
        } catch (const std::exception& e) {
            continue;
        }
    }
    for(int32_t i=0;i<j;i++){
        try{
            grpc::Status status_=status[i].get();
            if(!status_.ok() || !response[i].success())
            {
                continue;
            } 
            cnt++;
        } catch (const std::exception& e) {
            continue;
        }
    }
    if(cnt>(get_nodes_cnt()/2)) return true;
    return false;    
}

bool raftManager::broadcast_member_update(::member_request request){

    grpc::ClientContext context;
    std::vector<::member_response> response(mpp.size());
    std::vector<std::future<grpc::Status>> status;
    int cnt=0,j=0;
    for(auto &i : mpp){
        try {
            int32_t j_copy=j++;
            status.push_back(std::async
                (std::launch::async,
                    [&,j_copy](){

                        return retry(
                            [&](grpc::ClientContext* ctx, const ::member_request& req, ::member_response* res) {
                                return i.second.s2->update_cluster_member(ctx, req, res);
                            },
                            heartbeat_timeout,
                            &context, request, &(response[j_copy])
                        );
                    }
                )
            );
        } catch (const std::exception& e) {
            continue;
        }
    }
    for(int32_t i=0;i<j;i++){
        try{
            grpc::Status status_=status[i].get();
            if(!status_.ok() || !response[i].success())
            {
                continue;
            } 
            cnt++;
        } catch (const std::exception& e) {
            continue;
        }
    }

    if(cnt>(get_nodes_cnt()/2)) {
        share_cluster_info_with(request.ip_port(),request.cluster_key());
    
        return true;
    
    }
    return false;
}

void raftManager::start_heartbeat_sensing(){
    
    std::unique_lock<std::mutex> last_contact_lock(last_contact_mutex,std::defer_lock),
                                    heartbeat_lock(heartbeat_mutex,std::defer_lock),
                                    master_info_lock(master_info_mutex,std::defer_lock);
    stop_heartbeat_sensing();
    heartbeat_cv.wait(heartbeat_lock,[this](){
        return is_running==false;
    });
    is_running = true;
    run_heartbeat_sensing = true;
    heartbeat_lock.unlock();
    
    
    while(run_heartbeat_sensing){
        wait_for(heartbeat_timeout);
        if(get_state()==FOLLOWER){
            ::heart_request request;
            ::beats_response response;
            grpc::ClientContext context;
            master_info_lock.lock();
            request.set_term(term_id);
            grpc::Status status;
            if(master_stub!=nullptr){
                master_info_lock.unlock();
                status = retry(
                            [&](grpc::ClientContext* ctx, const ::heart_request& req, ::beats_response* res) {
                                return master_stub->heart_beat(ctx, req, res);
                            },
                            heartbeat_timeout,
                            &context, request, &(response)
                        );
            }
            else master_info_lock.unlock();
            master_info_lock.lock();
            if(status.ok()){
                if(response.is_master())
                {
                    master_info_lock.unlock();
                    continue;
                } else if(response.term()>term_id
                    && response.master_ip_port() != "")
                {
                    update_master(response.master_ip_port(),response.term());
                    master_info_lock.unlock();
                    continue;
                }
                else master_info_lock.unlock();
            }
            last_contact_lock.lock();
            auto diff=std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()-last_contact
            );
            last_contact_lock.unlock();
            if(diff<=heartbeat_timeout){
                continue;
            }
        }
        start_voting();
    }
    heartbeat_lock.lock();
    is_running = false;
    heartbeat_cv.notify_one();
}

void raftManager::start_voting(){
    std::unique_lock<std::mutex> lock(last_voted_mutex);
    auto diff=std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now()-last_voted
    );
    if(diff<=election_timeout)return;
    // std::string ""="";
    
    
    ::vote_request request;
    request.set_ip_port(this_ip_port);
    std::unique_lock<std::mutex> master_info_lock(master_info_mutex);
    int32_t term_id_=term_id;
    master_info_lock.unlock();
    change_state_to(CANDIDATE,"",term_id_+1);
    request.set_term(term_id_+1);
    grpc::ClientContext context;
    std::vector<::vote_response> response(mpp.size());
    std::vector<std::future<grpc::Status>> status;
    int cnt=0,j=0;
    for(auto &i : mpp){
        try {
            int32_t j_copy=j++;
            status.push_back(std::async
                (std::launch::async,
                    [&,j_copy](){
                        return retry(
                                [&](grpc::ClientContext* ctx, const ::vote_request& req, ::vote_response* res) {
                                    return i.second.s2->vote_rpc(ctx, req, res);
                                },
                                election_timeout,
                                &context, request, &(response[j_copy])
                            );
                    }
                )
            );
        } catch (const std::exception& e) {
            continue;
        }
    }
    for(int32_t i=0;i<j;i++){
        try{
            grpc::Status status_=status[i].get();
            if(!status_.ok())
            {
                continue;
            }
            if(response[i].vote()) cnt++;
        } catch (const std::exception& e) {
            continue;
        }
    }
    if(cnt>(get_nodes_cnt()/2)) {
        change_state_to(MASTER,this_ip_port,term_id_+1);
        broadcast_new_master();
        stop_heartbeat_sensing();
        grpc::ClientContext context;
    }
    else{
        master_info_lock.lock();
        change_state_to(FOLLOWER,"",term_id_);
        master_info_lock.unlock();
    }  
}

// assuming broadcast is guaranteed
void raftManager::broadcast_new_master(){
    ::master_change_request request;
    request.set_ip_port(this_ip_port);
    request.set_term(term_id);
    ::master_change_response response;
    grpc::ClientContext context;
    int cnt=0;
    for(auto &i : mpp){
        try {
            std::async(
                std::launch::async,
                [&](){
                    return retry(
                                [&](grpc::ClientContext* ctx, const ::master_change_request& req, ::master_change_response* res) {
                                    return i.second.s2->new_master(ctx, req, res);
                                },
                                heartbeat_timeout,
                                &context, request, &(response)
                            );
                }
            );         
        } catch (const std::exception& e) {
            continue;
        }
    }
}

raftManager::raftManager(int32_t election_timeout_,
                                    int32_t heartbeat_timeout_,
                                    std::string &this_ip_port_,
                                    int32_t max_retries_,
                                    std::string master_ip_port_,
                                    STATE state_
                                ){
    state=state_;
    election_timeout = std::chrono::milliseconds(election_timeout_);
    heartbeat_timeout = std::chrono::milliseconds(heartbeat_timeout_);
    max_retries = max_retries_;
    last_contact = std::chrono::system_clock::now();
    last_voted = last_contact;
    this_ip_port=this_ip_port_;
    if(master_ip_port_!=""){
        update_master(master_ip_port_,0);
    }
    
    
}

void raftManager::share_cluster_info_with(std::string ip_port,std::string cluster_key){
    std::vector<::member_request> request;
    ::member_response response;
    int32_t j=0;
    grpc::ClientContext context;
    auto it = mpp.find(ip_port);
    for(auto &i : mpp){
        request[j].set_cluster_key(cluster_key);
        request[j].set_to_drop(false);
        request[j].set_ip_port(i.first);
        int j_copy=j;
        try {
            std::async(
                std::launch::async,
                [&](){
                    return retry(
                                [&](grpc::ClientContext* ctx, const ::member_request& req, ::member_response* res) {
                                    return (it->second).s2->update_cluster_member(ctx, req, res);
                                },
                                heartbeat_timeout,
                                &context, request[j_copy], &(response)
                            );
                }
            );         
        } catch (const std::exception& e) {
            continue;
        }
        j++;
    }
    
}

