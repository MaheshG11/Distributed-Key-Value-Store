#pragma once
#ifndef RAFT_SERVER
#define RAFT_SERVER
#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include <bits/stdc++.h>
#include "log_store.h"
#include "raft_manager.h"

bool TRUE=true,FALSE=false;
template <typename T>
class raftServer : public raft::Service{

public:
    raftServer(std::shared_ptr<logStore<T>> log_store_ptr,
        raftManager &raft_manager,
        std::unique_lock<std::mutex> &raft_manager_lock,
        std::string cluster_key
    );
    ~raftServer();
    ::grpc::Status send_log_entry(::grpc::ServerContext* context, const ::T* request, ::log_response* response);
    ::grpc::Status commit_log_entry(::grpc::ServerContext* context, const ::commit_request* request, ::commit_response* response);
    ::grpc::Status heart_beat(::grpc::ServerContext* context, const ::heart_request* request, ::beats_response* response);
    ::grpc::Status vote_rpc(::grpc::ServerContext* context, const ::vote_request* request, ::vote_response* response);
    ::grpc::Status new_master(::grpc::ServerContext* context, const ::master_change_request* request, ::master_change_response* response);
    ::grpc::Status join_cluster(::grpc::ServerContext* context, const ::join_cluster_request* request, ::join_cluster_response* response);
    ::grpc::Status update_cluster_member(::grpc::ServerContext* context, const ::member_request* request, ::member_response* response);

private:
    std::string cluster_key;
    std::shared_ptr<logStore<T>> log_store_ptr;
    raftManager &raft_manager;
    std::unique_lock<std::mutex> &raft_manager_lock;

};
template <typename T>
raftServer::raftServer(std::shared_ptr<logStore<T>> log_store_ptr,
        raftManager &raft_manager,
        std::unique_lock<std::mutex> &raft_manager_lock,
        std::string cluster_key
    ):
        log_store_ptr(log_store_ptr),
        raft_manager(raft_manager),
        raft_manager_lock(raft_manager_lock)
        cluster_key(cluster_key){}

template <typename T>
::grpc::Status raftServer::send_log_entry(::grpc::ServerContext* context, const ::T* request, ::log_response* response){
    std::async(std::launch::async, log_store_ptr->append_entry, *request);
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer::commit_log_entry(::grpc::ServerContext* context, const ::commit_request* request, ::commit_response* response){
    raft_manager_lock.lock();
    if(raft_manager.term_id>(request->term_id()))
        {
            std::async(std::launch::async, log_store_ptr->commit, request->entry_id(), false);
            response->set_success(false);
        }
    else{
        std::async(std::launch::async, log_store_ptr->commit, request->entry_id(), request->commit());
        response->set_success(true);
    }
    raft_manager_lock.unlock();
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer::heart_beat(::grpc::ServerContext* context, const ::heart_request* request, ::beats_response* response){
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
::grpc::Status raftServer::vote_rpc(::grpc::ServerContext* context, const ::vote_request* request, ::vote_response* response){
    raft_manager_lock.lock();
    if(raft_manager.can_vote(request->term())){
        response->set_vote(TRUE);
        raft_manager.unlock();
        raft_manager.update_last_voted();
    }
    else{
        response->set_vote(FALSE);
        raft_manager.unlock();   
    }
    return grpc::Status::OK;
}

template <typename T>
::grpc::Status raftServer::new_master(::grpc::ServerContext* context, const ::master_change_request* request, ::master_change_response* response){
    raft_manager_lock.lock();
    if(raft_manager.get_master().second>=(request->term)){
        response->set_success(FALSE);
    }
    else{
        raft_manager.change_state_to(STATE::FOLLOWER,request->ip_port(),request->term());
        response->set_success(TRUE);
    }
    raft_manager_lock.unlock();
    return grpc::Status::OK;
}



template <typename T>
::grpc::Status raftServer::update_cluster_member(::grpc::ServerContext* context, const ::member_request* request, ::member_response* response){
    if((request->ip_port())==raft_manager.this_ip_port){
        response->set_success(TRUE);
        return grpc::Status::OK;
    }
    if((request->cluster_key())==cluster_key && (request->to_drop())==FALSE){
        raft_manager_lock.lock();
        if(raft_manager.mpp.find(request->ip_port())!=mpp.end()){
            response->set_success(FALSE);
            raft_manager_lock.unlock();
            return grpc::Status::OK;
        }
        raft_manager.add_node_to_cluster(request->ip_port());
        if((raft_manager.get_state())==STATE::MASTER){
            raft_manager_lock.unlock();
            std::async(std::launch::async,raft_manager.broadcast_member_update,(*request));
            
        }
        else raft_manager_lock.unlock();
    }
    response->set_success(TRUE);
    return grpc::Status::OK;
}



#endif