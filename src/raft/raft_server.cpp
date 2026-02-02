#include "raft_server.h"
#include <spdlog/spdlog.h>
#include <memory>
#include "cluster_manager.h"

using namespace std;

RaftServer::RaftServer(std::shared_ptr<RaftManager> raft_manager)
    : raft_parameters_(raft_manager->raft_parameters_),
      raft_manager_(raft_manager),
      raft_state_(raft_manager->raft_state_),
      rpc_calls_(raft_manager_->rpc_calls_),
      cluster_manager_(raft_manager_->cluster_manager_),
      election_(raft_manager_->election_),
      log_queue_(cluster_manager_->log_queue_) {
  SPDLOG_INFO("RaftServer(constructor): Enter");
}

grpc::Status RaftServer::SendLogEntry(grpc::ServerContext* context,
                                      const LogRequest* request,
                                      LogResponse* response) {
  SPDLOG_INFO("RaftServer::SendLogEntry: Enter");
  response->set_success(log_queue_->AppendEntries((*request)));
  response->set_success(
      min(response->success(), log_queue_->CommitEntry(request->commit_idx())));
  return grpc::Status::OK;
}

grpc::Status RaftServer::Heartbeat(grpc::ServerContext* context,
                                   const HeartRequest* request,
                                   BeatsResponse* response) {
  SPDLOG_INFO("RaftServer::Heartbeat: Enter");

  if (raft_state_->GetState() == STATE::LEADER) {
    response->set_is_leader(true);
    return grpc::Status::OK;
  } else {
    response->set_is_leader(false);
  }
  response->set_term(raft_state_->GetTerm());

  // Get leader info and only set if valid
  auto leader_info = raft_manager_->GetLeader();
  if (!leader_info.first.empty()) {
    response->set_leader_ip_port(leader_info.first);
  }

  return grpc::Status::OK;
}

grpc::Status RaftServer::VoteRPC(grpc::ServerContext* context,
                                 const VoteRequest* request,
                                 VoteResponse* response) {
  SPDLOG_INFO("RaftServer::VoteRPC: Enter");
  auto ip_port = request->ip_port();
  bool res = election_->CanVote(request->term(), request->last_commit_index(),
                                ip_port);
  response->set_success(res);

  return grpc::Status::OK;
}

grpc::Status RaftServer::NewLeader(grpc::ServerContext* context,
                                   const LeaderChangeRequest* request,
                                   LeaderChangeResponse* response) {
  SPDLOG_INFO("RaftServer::NewLeader: Enter");
  auto ip_port = request->ip_port();

  bool res = cluster_manager_->UpdateLeader(ip_port, request->term());

  SPDLOG_WARN("Result of update leader {}", (int)res);
  response->set_success(res);
  // }
  return grpc::Status::OK;
}

grpc::Status RaftServer::UpdateClusterMember(grpc::ServerContext* context,
                                             const MemberRequest* request,
                                             ClusterInfo* response) {
  SPDLOG_INFO("RaftServer::UpdateClusterMember: Enter");
  SPDLOG_WARN("RaftServer::UpdateClusterMember: ip_port:{} | broadcast:{} ",
              request->ip_port(), request->broadcast());

  cluster_manager_->AddNode(request->ip_port());
  if (request->broadcast()) {
    SPDLOG_WARN("Now Broadcasting");

    MemberRequest request_ele;
    request_ele.set_broadcast(false);
    request_ele.set_cluster_key(request->cluster_key());
    request_ele.set_ip_port(request->ip_port());
    rpc_calls_->BroadcastMemberUpdate(request_ele);
    SPDLOG_WARN("Broadcasting Done now sharing cluster info");

    rpc_calls_->GetClusterInfo(response, request->cluster_key());
    return grpc::Status::OK;
  }

  return grpc::Status::OK;
}
