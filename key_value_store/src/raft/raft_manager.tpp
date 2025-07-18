template <typename Func, typename... Args>
auto raftManager::retry(Func&& func,std::chrono::milliseconds &timeout ,Args&&... args)
        -> decltype(func(std::forward<Args>(args)...))
{
    int retries = 0;
    while (true) {
        try {
            auto future = std::async(std::launch::async, func, std::forward<Args>(args)...);
        if (future.wait_for(timeout) == std::future_status::ready) {
            return future.get();
        } else {
            throw std::runtime_error("Timeout occurred");
        }
        } catch (const std::exception& e) {
            if (++retries > max_retries) {
                throw;
            }
        }
    }
}



template <typename T>
bool raftManager::broadcast_log_entry(T &request){
    request.set_term(get_master().second);
    grpc::ClientContext context;
    std::vector<::log_response> response(mpp.size());
    std::vector<std::future<grpc::Status>> status(mpp.size());
    int cnt=0,j=0;
    for(auto &i : mpp){
        try {
            int32_t j_copy=j++;
            status[j_copy]=std::async
                (std::launch::async,
                    [&,j_copy](){
                        return retry(
                                [&](grpc::ClientContext* ctx, const T& req, ::log_response* res) {
                                    return i.second.s2->send_log_entry(ctx, req, res);
                                },
                                heartbeat_timeout,
                                &context, request, &(response[j_copy])
                            );
                    }
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
    if(cnt>(get_nodes_cnt()/2)) return true;
    return false;
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
    std::unique_lock<std::mutex> lock(master_info_mutex);
    master_ip_port=ip_port;
    term_id=term_;
    if(ip_port=="" || ip_port==this_ip_port){
        master_stub=nullptr;
        return;
    }
    master_stub=raft::NewStub(grpc::CreateChannel(
            ip_port, grpc::InsecureChannelCredentials()
    ));
}

inline bool raftManager::change_state_to(STATE state_, std::string master_ip_port,int32_t term_){
    std::unique_lock<std::mutex> lock(state_mutex);
    if(state==MASTER && state_==FOLLOWER){
        start_heartbeat_sensing();
    }
    state=state_;
    update_master(master_ip_port,term_);
    if(state==MASTER)stop_heartbeat_sensing();
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
}
inline bool raftManager::can_vote(int32_t term_id_){
    std::unique_lock<std::mutex> lock(last_voted_mutex),lock_(master_info_mutex);
    auto diff=std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()-last_voted
            );
    return (diff>election_timeout) && (term_id_>term_id);
}