#pragma once
#include <future>
#include <memory>
#include "cluster_manager.h"
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"
#include "raft_dtypes.h"

/**
 * @brief Make and manage all the RPC calls
 */
class RPCCalls {

 public:
  /**
  * @brief constructor
  * @param cluster_manager_ shared pointer of ClusterManager
  * @param raft_parameters_ shared pointer of RaftParameters
  * @param raft_state_ shared pointer of RaftState
  */
  RPCCalls(std::shared_ptr<RaftParameters> raft_parameters,
           std::shared_ptr<RaftState> raft_state,
           std::shared_ptr<ClusterManager> cluster_manager);

  RPCCalls() = delete;
  RPCCalls(const RPCCalls& other) = delete;
  RPCCalls(RPCCalls&& other) = default;
  RPCCalls& operator=(RPCCalls& other) = delete;
  RPCCalls& operator=(RPCCalls&& other) = default;
  /**
     * @brief broadcast log entry to all the other nodes
     * @param entry entry to broadcast 
     * @param success set value true if quorum agrees
     * @param success_fut future for success
     */
  void BroadcastLogEntry(::log_request& entry, std::promise<bool> success,
                         std::future<bool>& success_fut);

  /**
   * @brief broadcast commit with entry id
   * @param entry_id commit entry with this id
   * @param commit if true commit the entry else drop it     
   * @param success set value true if quorum agrees
   * @param success_fut future for success
   */
  void BroadcastCommit(int64_t entry_id, bool commit,
                       std::promise<bool> success,
                       std::future<bool>& success_fut);

  /**
     * @brief forward log entry to master
     * @param entry the entry to forward to master
     */
  bool ForwardLogEntry(::log_request entry);

  /**
   * @brief broadcast new leader to all the other nodes
   */
  void BroadcastNewLeader();

  /**
   * @brief broadcast if a member down or has joined the cluster
   */
  bool BroadcastMemberUpdate(::member_request request);

  /**
   * @brief send heartbeat to the leader
   */
  beats_response SendHeartbeat(heart_request& request);

  /**
   * @brief Share cluster info to the node with
   * @param ip_port address of the node to send cluster info with
   * @param cluster_key cluster auth key
   */
  bool ShareClusterInfo(std::string ip_port, std::string cluster_key);

  /**
   * @brief Requests votes from all other nodes 
   * @param votes reference to a atomic int32_t which counts how many votes has been 
   *  gathered during this vote request broadcast
   */
  void CollectVotes(std::promise<bool> won, std::future<bool>& won_fut);

  /**
   * @brief helper function for single request 
   */
  template <typename Func, typename... Args>
  bool Retry(Func&& func, std::chrono::milliseconds& timeout,
             grpc::Status& status, Args&&... args);

  /**
   * @brief helper function for broadcast request
   */
  template <typename Func, typename Response, typename Request>
  bool Retry(Func&& func, std::chrono::milliseconds& timeout,
             grpc::Status& status, std::mutex& prom_mtx,
             std::promise<bool>& prom, std::future<bool>& fut,
             std::atomic<int32_t>& votes, std::atomic<bool>& got_res,
             Response* response, Request& request);

  bool SendMemberRequest(std::string ip_port, bool broadcast, bool to_drop);

 private:
  std::shared_ptr<RaftParameters> raft_parameters_;
  std::shared_ptr<RaftState> raft_state_;
  std::shared_ptr<ClusterManager> cluster_manager_;
};