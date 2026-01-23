#pragma once
#include <spdlog/spdlog.h>
#include <chrono>
#include <memory>
#include <thread>
#include "cluster_manager.h"
#include "raft_dtypes.h"
#include "raft_manager_callback.h"
#include "rpc_calls.h"
class HeartbeatSensor {
 public:
  /**
   * @brief constructor
   * @param callback callback which invokes functions in raft_manager
   */
  HeartbeatSensor(RaftManagerCallback* callback);
  /**
     * @brief get the time when this node contacted most recently with the leader
     */
  inline std::chrono::time_point<std::chrono::system_clock> GetLastContact() {
    return last_contact_;
  }

  /**
   * @brief begin hearbeat sensing
   */
  void Start();

  /**
   * @brief stop hearbeat sensing
   */
  void Stop();

  /**
   * @brief update the last contact time to current time
   */
  inline void UpdateLastContact() {
    spdlog::info("HeartbeatSensor(UpdateLastContact): Enter");

    last_contact_ = std::chrono::system_clock::now();
  }

 private:
  /**
   * @brief begin hearbeat sensing
   */
  void start();

 private:
  std::shared_ptr<RPCCalls> rpc_calls_;
  std::shared_ptr<RaftState> raft_state_;
  std::shared_ptr<RaftParameters> raft_parameters_;
  std::shared_ptr<ClusterManager> cluster_manager_;
  RaftManagerCallback* callback_;
  std::atomic<bool> sense_;
  std::thread heartbeat_thread_;
  std::chrono::time_point<std::chrono::system_clock> last_contact_;
  std::shared_ptr<ApiImpl> api_impl_;
};