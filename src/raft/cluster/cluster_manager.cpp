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
  SPDLOG_INFO("ClusterManager(constructor): Enter");
  std::shared_ptr<std::string> leader_ip_port =
      make_shared<string>(string("null"));
  leader_ip_port_.store(leader_ip_port);
  log_queue_ = make_shared<RaftQueue>(raft_parameters->path);
}

/**
  * @brief Add Node to cluster 
  * @param ip_port address of the node to add to the cluster 
  * @returns true on success
  */
bool ClusterManager::AddNode(const string& ip_port) {
  SPDLOG_INFO("ClusterManager::AddNode: Enter {}", ip_port);

  auto iter = cluster_map_.find(ip_port);
  auto res = true;
  if (iter == cluster_map_.end()) {
    pair<string, NodeState> node_stub{ip_port, ip_port};
    res = cluster_map_.insert(move(node_stub)).second;

  } else {
    iter->second = NodeState(ip_port);
  }
  SPDLOG_INFO("ClusterManager::AddNode {} | res {}", cluster_map_.size(), res);

  return res;
}

/**
   * @brief get count of nodes in the cluster
   * @returns number of nodes in the cluster 
   */
int32_t ClusterManager::GetNodesCnt() {
  SPDLOG_INFO("ClusterManager::GetNodesCnt: Enter{}", cluster_map_.size());

  return cluster_map_.size();
}

/**
 * @brief Update leader with ip_port
 * @param ip_port address of the node to drop from the cluster 
 * @param term the new term of the new leader
 */
bool ClusterManager::UpdateLeader(const string& ip_port, int32_t term) {
  SPDLOG_INFO("ClusterManager::UpdateLeader: Enter {} | {} (this={})", ip_port,
              term, (void*)this);

  // Ignore empty ip_port updates
  if (ip_port.empty()) {
    SPDLOG_WARN(
        "ClusterManager::UpdateLeader: ip_port is empty, ignoring update");
    return false;
  }

  if (raft_state_->GetTerm() < term) {
    // log previous value for debugging
    {
      lock_guard<mutex> lock_prev(leader_mtx_);
      auto leader_ptr = leader_ip_port_.load(std::memory_order_acquire);
      SPDLOG_INFO("ClusterManager::UpdateLeader: previous leader '{}' (len={})",
                  *leader_ptr, (int)(*leader_ptr).size());
    }

    lock_guard<mutex> lock1(leader_mtx_);
    auto leader_ptr = leader_ip_port_.load(std::memory_order_acquire);
    (*leader_ptr) = ip_port;

    SPDLOG_INFO("ClusterManager::UpdateLeader: new leader '{}' (len={})",
                (*leader_ptr), (int)(*leader_ptr).size());

    raft_state_->SetLeaderAvailable(true);
    if (ip_port == raft_parameters_->this_ip_port) {
      raft_state_->SetState(LEADER);
      rpc_calls_->AppendLogEntries(api_impl_->commited_idx);
    } else {
      raft_state_->SetState(FOLLOWER);
      rpc_calls_->StopAppendentries();
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
  SPDLOG_INFO("ClusterManager::GetLeader: Enter (this={})", (void*)this);
  lock_guard<mutex> lock1(leader_mtx_);

  // Make a defensive copy inside the lock to prevent any data races
  auto leader_ptr = leader_ip_port_.load(std::memory_order_acquire);
  string leader_copy = *leader_ptr;
  int32_t term_copy = raft_state_->GetTerm();

  SPDLOG_INFO(
      "ClusterManager::GetLeader: Enter {} | {} (cluster_size={}, this={})",
      leader_copy, term_copy, cluster_map_.size(), (void*)this);
  SPDLOG_INFO("ClusterManager::GetLeader: leader length={} (raw) ",
              (int)leader_copy.size());
  std::cout << leader_copy << '\n';

  return {leader_copy, term_copy};
}

/**
   * @brief get leader details
   * @returns ip_port and its current term
   */
std::shared_ptr<Raft::Stub> ClusterManager::GetLeaderStub() {
  SPDLOG_INFO("ClusterManager::GetLeaderStub: Enter");

  lock_guard<mutex> lock1(leader_mtx_);

  // Check if *leader_ptr is empty
  auto leader_ptr = leader_ip_port_.load(std::memory_order_acquire);

  if ((*leader_ptr).empty()) {
    SPDLOG_WARN("ClusterManager::GetLeaderStub: *leader_ptr is empty!");
    return nullptr;  // Return null instead of crashing
  }

  auto it = cluster_map_.find(*leader_ptr);
  if (it == cluster_map_.end()) {
    SPDLOG_WARN(
        "ClusterManager::GetLeaderStub: *leader_ptr {} not in cluster map",
        *leader_ptr);
    return nullptr;  // Return null if not found
  }

  return it->second.s2;
}