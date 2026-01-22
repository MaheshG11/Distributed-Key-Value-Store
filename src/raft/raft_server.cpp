#include "raft_server.h"
#include <spdlog/spdlog.h>
#include <memory>

using namespace std;

RaftServer::RaftServer(std::shared_ptr<RaftManager> raft_manager)
    : raft_parameters_(raft_manager->raft_parameters_),
      raft_manager_(raft_manager),
      raft_state_(raft_manager->raft_state_),
      rpc_calls_(raft_manager_->rpc_calls_),
      cluster_manager_(raft_manager_->cluster_manager_),
      election_(raft_manager_->election_) {
  spdlog::info("RaftServer(constructor): Enter");
}

grpc::Status RaftServer::SendLogEntry(grpc::ServerContext* context,
                                      const LogRequest* request,
                                      LogResponse* response) {
  // log_store_ptr->append_entry(*request);
  spdlog::info("RaftServer::SendLogEntry: Enter");

  response->set_success(true);
  return grpc::Status::OK;
}

grpc::Status RaftServer::CommitLogEntry(grpc::ServerContext* context,
                                        const CommitRequest* request,
                                        CommitResponse* response) {
  spdlog::info("RaftServer::CommitLogEntry: Enter");

  // std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
  // if (raft_manager.get_term_id() > (request->term())) {
  //   std::async(
  //       std::launch::async,
  //       [&](int32_t entry_id_, bool commit) {
  //         log_store_ptr->commit(entry_id_, commit);
  //       },
  //       request->entry_id(), FALSE);
  //   response->set_success(FALSE);
  // } else {
  //   std::async(
  //       std::launch::async,
  //       [&](int32_t entry_id_, bool commit) {
  //         log_store_ptr->commit(entry_id_, commit);
  //       },
  //       request->entry_id(), request->commit());
  //   response->set_success(TRUE);
  //   raft_manager.update_last_contact();
  // }
  return grpc::Status::OK;
}

grpc::Status RaftServer::Heartbeat(grpc::ServerContext* context,
                                   const HeartRequest* request,
                                   BeatsResponse* response) {
  spdlog::info("RaftServer::Heartbeat: Enter");

  if (raft_state_->GetState() == STATE::LEADER) {
    response->set_is_leader(true);
    return grpc::Status::OK;
  } else {
    response->set_is_leader(false);
  }
  response->set_term(raft_state_->GetTerm());
  response->set_leader_ip_port(raft_manager_->GetLeader().first);
  return grpc::Status::OK;
}

grpc::Status RaftServer::VoteRPC(grpc::ServerContext* context,
                                 const VoteRequest* request,
                                 VoteResponse* response) {
  spdlog::info("RaftServer::VoteRPC: Enter");
  auto ip_port = request->ip_port();
  bool res = election_->CanVote(request->term(), request->last_commit_index(),
                                ip_port);
  response->set_success(res);

  // std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
  // std::cout << "someone ask)ed for a vote : ";
  // if (raft_manager.can_vote(request->term())) {
  //   response->set_vote(TRUE);
  //   raft_manager_lock.unlock();
  //   raft_manager.update_last_voted();
  //   std::cout << "I replied yes\n";

  // } else {
  //   response->set_vote(FALSE);
  //   std::cout << "I replied no\n";
  // }
  return grpc::Status::OK;
}

grpc::Status RaftServer::NewLeader(grpc::ServerContext* context,
                                   const LeaderChangeRequest* request,
                                   LeaderChangeResponse* response) {
  spdlog::info("RaftServer::NewLeader: Enter");
  auto ip_port = request->ip_port();

  bool res = cluster_manager_->UpdateLeader(ip_port, request->term());
  spdlog::warn("Result of update leader {}", (int)res);
  response->set_success(res);
  // std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
  // std::cout << "someone with " << (request->ip_port())
  //           << " says they are the new leader their term id is : "
  //           << (request->term());
  // if ((raft_manager.get_term_id() == (request->term()) &&
  //      raft_manager.get_state() != CANDIDATE) ||
  //     raft_manager.get_term_id() > (request->term())) {
  //   response->set_success(FALSE);
  // } else {
  //   std::cout << "Found new leader : " << (request->ip_port()) << '\n';
  //   raft_manager.change_state_to(
  //       STATE::FOLLOWER, std::string(request->ip_port()), request->term());
  //   response->set_success(TRUE);
  // }
  return grpc::Status::OK;
}

grpc::Status RaftServer::UpdateClusterMember(grpc::ServerContext* context,
                                             const MemberRequest* request,
                                             MemberResponse* response) {
  spdlog::info("RaftServer::UpdateClusterMember: Enter");
  spdlog::warn("RaftServer::UpdateClusterMember: ip_port:{} | broadcast:{} ",
               request->ip_port(), request->broadcast());

  cluster_manager_->AddNode(request->ip_port());
  if (request->broadcast()) {
    spdlog::warn("Now Broadcasting");

    MemberRequest request_ele;
    request_ele.set_broadcast(false);
    request_ele.set_cluster_key(request->cluster_key());
    request_ele.set_ip_port(request->ip_port());
    rpc_calls_->BroadcastMemberUpdate(request_ele);
    rpc_calls_->ShareClusterInfo(request->ip_port(), request->cluster_key());
    return grpc::Status::OK;
  }

  // if ((request->cluster_key()) == cluster_key &&
  //     (request->to_drop()) == FALSE) {
  //   std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
  //   response->set_success(TRUE);
  //   if (raft_manager.mpp.find(request->ip_port()) != raft_manager.mpp.end()) {
  //     raft_manager.add_node_to_cluster(request->ip_port());
  //     std::thread(
  //         [&](std::string ip_port, std::string key_) {
  //           raft_manager.share_cluster_info_with(ip_port, cluster_key);
  //         },
  //         request->ip_port(), request->cluster_key())
  //         .detach();
  //     return grpc::Status::OK;
  //   }
  //   raft_manager.add_node_to_cluster(request->ip_port());
  //   if ((raft_manager.get_state()) == STATE::LEADER) {
  //     raft_manager_lock.unlock();
  //     std::thread(
  //         [&](MemberRequest request) {
  //           raft_manager.broadcast_member_update(request);
  //         },
  //         (*request))
  //         .detach();
  //   } else
  //     raft_manager_lock.unlock();
  //   std::cout << "added Node to cluster with ip:port :: "
  //             << (request->ip_port()) << std::endl;
  // } else {
  //   response->set_success(FALSE);
  // }

  return grpc::Status::OK;
}

grpc::Status RaftServer::ShareClusterInfo(grpc::ServerContext* context,
                                          const ::ClusterInfo* request,
                                          CommitResponse* response) {
  spdlog::info("RaftServer::ShareClusterInfo: Enter");

  for (const auto& ip_port : request->ip_port()) {
    cluster_manager_->AddNode(ip_port);
  }
  string leader_ip_port = request->leader_ip_port();
  cluster_manager_->UpdateLeader(leader_ip_port, request->term());
  return grpc::Status::OK;
}
