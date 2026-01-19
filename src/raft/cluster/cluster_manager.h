#pragma once
#include <memory>
#include "raft_dtypes.h"
#include "raft_state.h"
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
  * @brief drop Node to cluster 
  * @param ip_port address of the node to drop from the cluster 
  * @returns true on success
  */
  bool DropNode(const std::string& ip_port);

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
  std::unique_ptr<raft::Stub>& GetLeaderStub();

  /**
   * @brief get leader details
   * @returns ip_port and its current term
   */
  std::mutex& GetLeaderMutex();

  /*
    custom iterator
  */
  using iterator = std::map<std::string, STUB>::iterator;
  using const_iterator = std::map<std::string, STUB>::const_iterator;
  inline iterator begin() { return cluster_map_.begin(); }
  inline iterator end() { return cluster_map_.end(); }

  inline const_iterator begin() const { return cluster_map_.begin(); }
  inline const_iterator end() const { return cluster_map_.end(); }

  inline iterator Find(std::string& ip_port) {
    return cluster_map_.find(ip_port);
  }

 private:
  std::map<std::string, STUB> cluster_map_;
  std::unique_ptr<raft::Stub> leader_stub_;
  std::mutex leader_stub_mtx_;
  std::string leader_ip_port_ = "", cluster_key_;
  std::shared_ptr<RaftParameters> raft_parameters_;
  std::shared_ptr<RaftState> raft_state_;
};