template <typename T>
raftServer<T>::raftServer(std::shared_ptr<logStore<T>> log_store_ptr,
        raftManager &raft_manager,
        std::unique_lock<std::mutex> &raft_manager_lock,
        std::string cluster_key
    ):
        log_store_ptr(log_store_ptr),
        raft_manager(raft_manager),
        raft_manager_lock(raft_manager_lock),
        cluster_key(cluster_key){}

template <typename T>
::grpc::Status raftServer<T>::send_log_entry(::grpc::ServerContext* context, const T* request, ::log_response* response){
    log_store_ptr->append_entry(*request);
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer<T>::commit_log_entry(::grpc::ServerContext* context, const ::commit_request* request, ::commit_response* response){
    raft_manager_lock.lock();
    if(raft_manager.term_id>(request->term()))
        {
            std::async(std::launch::async, [&](int32_t entry_id_,bool commit){
                log_store_ptr->commit(entry_id_,commit);
            }, request->entry_id(), false);
            response->set_success(false);
        }
    else{
        std::async(std::launch::async, [&](int32_t entry_id_,bool commit){
            log_store_ptr->commit(entry_id_,commit);
        }, request->entry_id(), request->commit());
        response->set_success(true);
    }
    raft_manager_lock.unlock();
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer<T>::heart_beat(::grpc::ServerContext* context, const ::heart_request* request, ::beats_response* response){
    raft_manager_lock.lock();
    if(raft_manager.get_state()==STATE::MASTER){
        response->set_is_master(true);

    } else{
        response->set_is_master(false);
    }
    response->set_term(raft_manager.term_id);
    response->set_master_ip_port((raft_manager.get_master()).first);
    raft_manager_lock.unlock();
    return grpc::Status::OK;

}

template <typename T>
::grpc::Status raftServer<T>::vote_rpc(::grpc::ServerContext* context, const ::vote_request* request, ::vote_response* response){
    raft_manager_lock.lock();
    if(raft_manager.can_vote(request->term())){
        response->set_vote(TRUE);
        raft_manager_lock.unlock();
        raft_manager.update_last_voted();
    }
    else{
        response->set_vote(FALSE);
        raft_manager_lock.unlock();   
    }
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer<T>::new_master(::grpc::ServerContext* context, const ::master_change_request* request, ::master_change_response* response){
    raft_manager_lock.lock();
    if(raft_manager.get_master().second>=(request->term())){
        response->set_success(FALSE);
    }
    else{
        raft_manager.change_state_to(STATE::FOLLOWER,std::string(request->ip_port()),request->term());
        response->set_success(TRUE);
    }
    raft_manager_lock.unlock();
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer<T>::update_cluster_member(::grpc::ServerContext* context, const ::member_request* request, ::member_response* response){
    if((request->ip_port())==raft_manager.this_ip_port){
        response->set_success(TRUE);
        return grpc::Status::OK;
    }
    if((request->cluster_key())==cluster_key && (request->to_drop())==FALSE){
        raft_manager_lock.lock();
        if(raft_manager.mpp.find(request->ip_port())!=raft_manager.mpp.end()){
            response->set_success(FALSE);
            raft_manager_lock.unlock();
            return grpc::Status::OK;
        }
        raft_manager.add_node_to_cluster(std::string(request->ip_port()));
        if((raft_manager.get_state())==STATE::MASTER){
            raft_manager_lock.unlock();
            std::async(std::launch::async,[&](member_request request){
                raft_manager.broadcast_member_update(request);
            },(*request));
            
        }
        else raft_manager_lock.unlock();
    }
    response->set_success(TRUE);
    return grpc::Status::OK;
}

