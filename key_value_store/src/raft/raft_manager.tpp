template <typename Func, typename... Args>
    
void raftManager::retry(Func&& func, std::chrono::milliseconds& timeout, grpc::Status& status, Args&&... args)
{   
    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(100)+timeout;  // 500 ms timeout
    
    int retries = 0;
    while (retries<max_retries) {
        grpc::ClientContext context;
        deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(100)+timeout;  // 500 ms timeout
        context.set_deadline(deadline);
        status=func(&context,std::forward<Args>(args)...);
        if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED){
            retries++;
        }
        else if(status.error_code() == grpc::StatusCode::UNAVAILABLE){
            retries++;
            wait_for(timeout);
        }
        else break;
    }
}



template <typename T>
bool raftManager::broadcast_log_entry(T &request){
    request.set_term(get_master().second);
    grpc::ClientContext context;
    std::vector<::log_response> response(mpp.size());
    std::vector<grpc::Status> status(mpp.size());
    std::vector<std::future<void>> futures(mpp.size());
    int cnt=0,j=0;
    for(auto &i : mpp){
        int32_t j_copy=j++;
        futures[j_copy]=std::async
            (std::launch::async,
                [&,j_copy](){
                    return retry([&](grpc::ClientContext* ctx, const log_request& req, log_response* res) {
                        return i.second.s2->send_log_entry(ctx, req, res);
                    },heartbeat_timeout,(status[j_copy]),
                            request, &(response[j_copy])
                        );
                }
            );

    }
    for(int32_t i=0;i<j;i++){
        try{
            futures[i].get();
            if(!status[i].ok())
            {
                continue;
            } else if(!response[i].success()){
                if(response[i].term()>term_id && response[i].master_ip_port()!="" ){
                    
                    update_master(response[i].master_ip_port(),response[i].term());
                    return false;
                }
                continue;
            }
            cnt++;
        } catch (const std::exception& e){
            continue;
        }
    }
    if(cnt>=(get_nodes_cnt()/2)) return true;
    return false;
}

template <typename T>
bool raftManager::forward_log_entry(T request){
    std::unique_lock<std::mutex> lock(master_info_mutex);
    grpc::ClientContext context;
    ::log_response response;
    master_stub->send_log_entry(&context,request,&response);
    return response.success();
}



inline STATE raftManager::get_state(){
    std::unique_lock<std::mutex> lock(state_mutex);
    return state;
}

inline void raftManager::wait_for(std::chrono::milliseconds &timeout){
        std::this_thread::sleep_for(timeout);
}

inline int32_t raftManager::get_nodes_cnt(){
    return (mpp.size()+1);
}

inline void raftManager::update_master(std::string ip_port,int32_t term_){
    std::cout<<"Acquiring master info llock\n";
    std::unique_lock<std::mutex> lock(master_info_mutex);
    std::cout<<"Acquired master info llock\n";
    master_ip_port=ip_port;
    term_id=term_;
    if(ip_port==""){
        master_stub=nullptr;
        return;
    }
    master_stub=raft::NewStub(grpc::CreateChannel(
            ip_port, grpc::InsecureChannelCredentials()
    ));
}

inline bool raftManager::change_state_to(STATE state_, std::string master_ip_port,int32_t term_){
    std::cout<<"acquiring state lock\n";
    std::unique_lock<std::mutex> lock(state_mutex);
    std::cout<<"Acquired state info llock\n";
    if(state!=FOLLOWER && state_==FOLLOWER){
        std::cout<<"I am the Follower \n";
        update_last_contact();
        start_heartbeat_sensing();
    }
    state=state_;
    update_master(master_ip_port,term_);
    if(state!=FOLLOWER){
        stop_heartbeat_sensing();
    }
    return true;
}
inline std::pair<std::string,int32_t> raftManager::get_master(){
    std::unique_lock<std::mutex> lock(master_info_mutex);
    return std::make_pair(master_ip_port,term_id);
}

inline void raftManager::stop_heartbeat_sensing(){
    std::unique_lock<std::mutex> lock(heartbeat_mutex);
    run_heartbeat_sensing = false;
}
inline void raftManager::update_last_contact(){
    std::unique_lock<std::mutex> lock(last_contact_mutex);
    last_contact = std::chrono::system_clock::now();
}
inline void raftManager::update_last_voted(){
    std::unique_lock<std::mutex> lock(last_voted_mutex);
    last_contact = std::chrono::system_clock::now();
    lock.unlock();
}

inline int32_t raftManager::get_term_id(){
    std::unique_lock<std::mutex> lock_(master_info_mutex);
    return term_id;
}


