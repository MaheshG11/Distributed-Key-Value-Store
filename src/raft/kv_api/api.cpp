#include <grpcpp/grpcpp.h>

#include <string>
#include "api.h"
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"

using namespace std;
ApiImpl::ApiImpl(shared_ptr<RaftQueue> log_queue,
                 std::shared_ptr<RaftState> raft_state)
    : log_queue_(log_queue), raft_state_(raft_state) {
  spdlog::info("ApiImpl::ApiImpl(constructor): Enter");
}

grpc::Status ApiImpl::GetRPC(grpc::ServerContext* context,
                             const StoreRequest* request,
                             StoreResponse* response) {
  spdlog::info("ApiImpl::GetRPC: Enter");

  std::string key = request->key();
  std::pair<std::string, bool> res = log_queue_->GetValue((*request));
  response->set_value(res.first);
  response->set_ok((bool)res.second);
  if (res.second) {
    return grpc::Status::OK;
  }
  return grpc::Status(grpc::StatusCode::NOT_FOUND, "key not found");
}

grpc::Status ApiImpl::WriteRPC(grpc::ServerContext* context,
                               const StoreRequest* request,
                               StoreResponse* response) {
  spdlog::info("ApiImpl::WriteRPC: Enter");

  if (raft_state_->GetState() == LEADER) {
    response->set_ok(log_queue_->AppendEntry((*request)));
  } else {
    response->set_ok(false);
    response->set_key("not a leader");
    response->set_value("not a leader");
  }
  return grpc::Status::OK;
}
