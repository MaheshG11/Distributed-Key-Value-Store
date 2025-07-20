template <typename T>
raftServer<T>::raftServer(std::shared_ptr<logStore<T>> log_store_ptr,
        raftManager &raft_manager,
        std::mutex &raft_manager_mutex,
        std::string cluster_key
    ):
        log_store_ptr(log_store_ptr),
        raft_manager(raft_manager),
        raft_manager_mutex(raft_manager_mutex),
        cluster_key(cluster_key){}

template <typename T>
::grpc::Status raftServer<T>::send_log_entry(::grpc::ServerContext* context, const T* request, ::log_response* response){
    log_store_ptr->append_entry(*request);
    response->set_success(true);
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer<T>::commit_log_entry(::grpc::ServerContext* context, const ::commit_request* request, ::commit_response* response){
    std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
    if(raft_manager.get_term_id()>(request->term()))
        {
            std::async(std::launch::async, [&](int32_t entry_id_,bool commit){
                log_store_ptr->commit(entry_id_,commit);
            }, request->entry_id(), FALSE);
            response->set_success(FALSE);
        }
    else{
        std::async(std::launch::async, [&](int32_t entry_id_,bool commit){
            log_store_ptr->commit(entry_id_,commit);
        }, request->entry_id(), request->commit());
        response->set_success(TRUE);
        raft_manager.update_last_contact();
    }
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer<T>::heart_beat(::grpc::ServerContext* context, const ::heart_request* request, ::beats_response* response){
    std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
    std::cout<<"got heartbeat request\n";
    if(raft_manager.get_state()==STATE::MASTER){
        response->set_is_master(TRUE);
        return grpc::Status::OK;
    } else{
        response->set_is_master(FALSE);
    }
    response->set_term(raft_manager.get_term_id());
    response->set_master_ip_port((raft_manager.get_master()).first);
    return grpc::Status::OK;

}

template <typename T>
::grpc::Status raftServer<T>::vote_rpc(::grpc::ServerContext* context, const ::vote_request* request, ::vote_response* response){
    std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
    std::cout<<"someone asked for a vote : ";
    if(raft_manager.can_vote(request->term())){
        response->set_vote(TRUE);
        raft_manager_lock.unlock();
        raft_manager.update_last_voted();
        std::cout<<"I replied yes\n";

    }
    else{
        response->set_vote(FALSE);  
        std::cout<<"I replied no\n";

    }
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer<T>::new_master(::grpc::ServerContext* context, const ::master_change_request* request, ::master_change_response* response){
    std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
    std::cout<<"someone with "<<(request->ip_port())<<" says they are the new master their term id is : "<<(request->term());
    if((raft_manager.get_term_id()==(request->term()) && raft_manager.get_state()!=CANDIDATE)
        || raft_manager.get_term_id()>(request->term())){
        response->set_success(FALSE);
    }
    else{
        std::cout<<"Found new master : "<<(request->ip_port())<<'\n';
        raft_manager.change_state_to(STATE::FOLLOWER,std::string(request->ip_port()),request->term());
        response->set_success(TRUE);
    }
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer<T>::update_cluster_member(::grpc::ServerContext* context, const ::member_request* request, ::member_response* response){
    if((request->ip_port())==raft_manager.this_ip_port){
        response->set_success(TRUE);
        return grpc::Status::OK;
    }
    if((request->cluster_key())==cluster_key && (request->to_drop())==FALSE){
        std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
        response->set_success(TRUE);        
        if(raft_manager.mpp.find(request->ip_port())!=raft_manager.mpp.end()){
            raft_manager.add_node_to_cluster(request->ip_port());
            std::thread([&](std::string ip_port,std::string key_){
                raft_manager.share_cluster_info_with(ip_port,cluster_key);
            },request->ip_port(),request->cluster_key()).detach();
            return grpc::Status::OK;
        }
        raft_manager.add_node_to_cluster(request->ip_port());
        if((raft_manager.get_state())==STATE::MASTER){
            raft_manager_lock.unlock();
            std::thread([&](member_request request){
                raft_manager.broadcast_member_update(request);
            },(*request)).detach();
        }
        else raft_manager_lock.unlock();
        std::cout<<"added Node to cluster with ip:port :: "<<(request->ip_port())<<std::endl;
    }
    else{
        response->set_success(FALSE);
    }
    
    return grpc::Status::OK;
}

