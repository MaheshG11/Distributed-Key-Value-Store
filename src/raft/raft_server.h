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
class RaftServer : public Raft::Service {

 public:
  RaftServer(std::shared_ptr<RaftManager> raft_manager);
  grpc::Status SendLogEntry(grpc::ServerContext* context,
                            const LogRequest* request,
                            LogResponse* response) override;
  grpc::Status CommitLogEntry(grpc::ServerContext* context,
                              const CommitRequest* request,
                              CommitResponse* response) override;
  grpc::Status Heartbeat(grpc::ServerContext* context,
                         const HeartRequest* request,
                         BeatsResponse* response) override;
  grpc::Status VoteRPC(grpc::ServerContext* context, const VoteRequest* request,
                       VoteResponse* response) override;
  grpc::Status NewLeader(grpc::ServerContext* context,
                         const LeaderChangeRequest* request,
                         LeaderChangeResponse* response) override;
  grpc::Status UpdateClusterMember(grpc::ServerContext* context,
                                   const MemberRequest* request,
                                   MemberResponse* response) override;
  grpc::Status ShareClusterInfo(grpc::ServerContext* context,
                                const ::ClusterInfo* request,
                                CommitResponse* response) override;

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
