#pragma once
#include <atomic>
#include <memory>
#include "api.h"
#include "raft_dtypes.h"
#include "raft_queue.h"
#include "raft_state.h"
#include "rpc_calls.h"

class RPCCalls;
/**
 * @brief Manage connections
 */
class ClusterManager {
 public:
  /**
  * @brief constructor
  * @param raft_parameters shared pointer of RaftParameters
  * @param raft_state shared pointer of RaftState
  */
  ClusterManager(std::shared_ptr<RaftParameters> raft_parameters,
                 std::shared_ptr<RaftState> raft_state);

  ClusterManager() = delete;
  ClusterManager(const ClusterManager& other) = delete;
  ClusterManager(ClusterManager&& other) = default;
  ClusterManager& operator=(ClusterManager& other) = delete;
  ClusterManager& operator=(ClusterManager&& other) = default;

  /**
  * @brief Add Node to cluster 
  * @param ip_port address of the node to add to the cluster 
  * @returns true on success
  */
  bool AddNode(const std::string& ip_port);

  /**
   * @brief get count of nodes in the cluster
   * @returns number of nodes in the cluster 
   */
  int32_t GetNodesCnt();

  /**
   * @brief Update leader with ip_port
  * @param ip_port address of the node to drop from the cluster 
  * @param term the new term of the new leader
   * 
   */
  bool UpdateLeader(const std::string& ip_port, int32_t term);

  /**
   * @brief get leader details
   * @returns ip_port and its current term
   */
  std::pair<std::string, int32_t> GetLeader();

  /**
   * @brief get leader details
   * @returns ip_port and its current term
   */
  std::shared_ptr<Raft::Stub> GetLeaderStub();

  /**
   * @brief get leader details
   * @returns ip_port and its current term
   */
  std::mutex& GetCommMutex();

  /**
   * sets rpc calls for cluster manager
   * @brief 
   * */
  inline void SetRPCCalls(std::shared_ptr<RPCCalls> rpc_calls) {
    rpc_calls_ = rpc_calls;
  }

  inline void SetApiImpl(std::shared_ptr<ApiImpl> api_impl) {
    api_impl_ = api_impl;
  }

  /*
    custom iterator
  */
  using iterator = std::map<std::string, NodeState>::iterator;
  using const_iterator = std::map<std::string, NodeState>::const_iterator;
  inline iterator begin() { return cluster_map_.begin(); }
  inline iterator end() { return cluster_map_.end(); }

  inline const_iterator begin() const { return cluster_map_.begin(); }
  inline const_iterator end() const { return cluster_map_.end(); }

  inline iterator Find(const std::string& ip_port) {
    return cluster_map_.find(ip_port);
  }

  inline size_t Size() { return cluster_map_.size(); };

  friend class RPCCalls;
  friend class RaftManager;
  friend class RaftServer;

 private:
  std::map<std::string, NodeState> cluster_map_;
  std::mutex leader_mtx_, cluster_map_mtx_;
  std::string cluster_key_;
  std::atomic<std::shared_ptr<std::string>> leader_ip_port_;
  std::shared_ptr<RaftParameters> raft_parameters_;
  std::shared_ptr<RaftState> raft_state_;
  std::shared_ptr<RaftQueue> log_queue_;
  std::shared_ptr<RPCCalls> rpc_calls_;
  std::shared_ptr<ApiImpl> api_impl_;
  // std::atomic<int64_t>& commit_idx_;
};