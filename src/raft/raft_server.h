#pragma once
#include <grpcpp/grpcpp.h>
#include "cluster_manager.h"
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"
// #include "log_store.h"
#include "raft_manager.h"
#include "raft_state.h"
#include "rpc_calls.h"
// bool TRUE = true, FALSE = false;
class RaftServer : public raft::Service {

 public:
  RaftServer(std::shared_ptr<RaftManager> raft_manager);
  grpc::Status send_log_entry(grpc::ServerContext* context,
                              const log_request* request,
                              log_response* response) override;
  grpc::Status commit_log_entry(grpc::ServerContext* context,
                                const commit_request* request,
                                commit_response* response) override;
  grpc::Status heart_beat(grpc::ServerContext* context,
                          const heart_request* request,
                          beats_response* response) override;
  grpc::Status vote_rpc(grpc::ServerContext* context,
                        const vote_request* request,
                        vote_response* response) override;
  grpc::Status new_leader(grpc::ServerContext* context,
                          const leader_change_request* request,
                          leader_change_response* response) override;
  grpc::Status update_cluster_member(grpc::ServerContext* context,
                                     const member_request* request,
                                     member_response* response) override;
  grpc::Status share_cluster_info(grpc::ServerContext* context,
                                  const ::cluster_info* request,
                                  commit_response* response) override;

 private:
  std::shared_ptr<RaftParameters> raft_parameters_;
  // std::shared_ptr<logStore<T>> log_store_ptr;
  std::shared_ptr<RaftManager> raft_manager_;
  std::shared_ptr<RaftState> raft_state_;
  std::shared_ptr<RPCCalls> rpc_calls_;
  std::shared_ptr<ClusterManager> cluster_manager_;
  std::shared_ptr<Election> election_;

  // std::mutex& raft_manager_mutex;
};
