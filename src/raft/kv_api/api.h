#pragma once
#include <grpcpp/grpcpp.h>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"
// #include "store.h"
#include <memory>
#include <string>
#include "log_queue.h"
#include "raft_state.h"
// #include "log_store_impl.cpp"

class ApiImpl : public KeyValueStoreRPC::Service {

 public:
  ApiImpl(std::shared_ptr<RaftQueue> raft_queue,
          std::shared_ptr<RaftState> raft_state);
  grpc::Status WriteRPC(grpc::ServerContext* context,
                        const StoreRequest* request, StoreResponse* response);
  grpc::Status GetRPC(grpc::ServerContext* context, const StoreRequest* request,
                      StoreResponse* response);

  std::atomic<int64_t> commited_idx;

 private:
  std::shared_ptr<RaftQueue> log_queue_;
  std::shared_ptr<RaftState> raft_state_;
};
