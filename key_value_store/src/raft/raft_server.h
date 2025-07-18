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
    ::grpc::Status send_log_entry(::grpc::ServerContext* context, const T* request, ::log_response* response);
    ::grpc::Status commit_log_entry(::grpc::ServerContext* context, const ::commit_request* request, ::commit_response* response);
    ::grpc::Status heart_beat(::grpc::ServerContext* context, const ::heart_request* request, ::beats_response* response);
    ::grpc::Status vote_rpc(::grpc::ServerContext* context, const ::vote_request* request, ::vote_response* response);
    ::grpc::Status new_master(::grpc::ServerContext* context, const ::master_change_request* request, ::master_change_response* response);
    ::grpc::Status update_cluster_member(::grpc::ServerContext* context, const ::member_request* request, ::member_response* response);

private:
    std::string cluster_key;
    std::shared_ptr<logStore<T>> log_store_ptr;
    raftManager &raft_manager;
    std::unique_lock<std::mutex> &raft_manager_lock;

};

#include "raft_server.tpp"
#endif