#include "cluster_manager.h"
#include "grpcpp/grpcpp.h"

#include <spdlog/spdlog.h>
#include <map>
#include <utility>
using namespace std;

/**
  * @brief constructor
  * @param raft_parameters shared pointer of RaftParameters
  * @param raft_state shared pointer of RaftState
  */
ClusterManager::ClusterManager(shared_ptr<RaftParameters> raft_parameters,
                               shared_ptr<RaftState> raft_state)
    : raft_parameters_(raft_parameters), raft_state_(raft_state) {
  spdlog::info("ClusterManager(constructor): Enter");
}

/**
  * @brief Add Node to cluster 
  * @param ip_port address of the node to add to the cluster 
  * @returns true on success
  */
bool ClusterManager::AddNode(const string& ip_port) {

  auto iter = cluster_map_.find(ip_port);
  auto res = true;
  if (iter == cluster_map_.end()) {
    pair<string, STUB> node_stub{ip_port, ip_port};
    res = cluster_map_.insert(move(node_stub)).second;

  } else {
    iter->second = STUB(ip_port);
  }
  spdlog::info("ClusterManager::AddNode {} | res {}", cluster_map_.size(), res);

  return res;
}

/**
  * @brief drop Node to cluster 
  * @param ip_port address of the node to drop from the cluster 
  * @returns true on success
  */
bool ClusterManager::DropNode(const string& ip_port) {

  auto res = cluster_map_.erase(ip_port);
  spdlog::info("ClusterManager::DropNode {}", cluster_map_.size());

  if (res >= 0)
    return true;
  return false;
}

/**
   * @brief get count of nodes in the cluster
   * @returns number of nodes in the cluster 
   */
int32_t ClusterManager::GetNodesCnt() {
  spdlog::info("ClusterManager::GetNodesCnt: Enter{}", cluster_map_.size());

  return cluster_map_.size();
}

/**
 * @brief Update leader with ip_port
 * @param ip_port address of the node to drop from the cluster 
 * @param term the new term of the new leader
 */
bool ClusterManager::UpdateLeader(const string& ip_port, int32_t term) {
  spdlog::info("ClusterManager::UpdateLeader: Enter {} | {}", ip_port, term);

  if (raft_state_->GetTerm() < term) {

    lock_guard<mutex> lock1(leader_stub_mtx_);
    leader_ip_port_ = ip_port;
    try {
      leader_stub_ = raft::NewStub(
          grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
    } catch (const exception& e) {
      leader_stub_ = nullptr;
    }
    if (ip_port == raft_parameters_->this_ip_port) {
      raft_state_->SetState(LEADER);
    } else {
      raft_state_->SetState(FOLLOWER);
    }
    raft_state_->SetTerm(term);
    return true;
  }
  return false;
}

/**
 * @brief get leader details
 * @returns ip_port and its current term
 */
pair<string, int32_t> ClusterManager::GetLeader() {
  spdlog::info("ClusterManager::GetLeader: Enter");
  spdlog::info("ClusterManager::UpdateLeader: Enter {} | {}", leader_ip_port_,
               raft_state_->GetTerm());

  lock_guard<mutex> lock1(leader_stub_mtx_);
  return {leader_ip_port_, raft_state_->GetTerm()};
}

/**
   * @brief get leader details
   * @returns ip_port and its current term
   */
std::unique_ptr<raft::Stub>& ClusterManager::GetLeaderStub() {
  spdlog::info("ClusterManager::GetLeaderStub: Enter");

  return leader_stub_;
}

/**
   * @brief get leader details
   * @returns ip_port and its current term
   */
std::mutex& ClusterManager::GetLeaderMutex() {
  spdlog::info("ClusterManager::GetLeaderMutex: Enter");

  return leader_stub_mtx_;
}