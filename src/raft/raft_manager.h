#pragma once

#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include "cluster_manager.h"
#include "election.h"
#include "heartbeat_sensor.h"
#include "raft_dtypes.h"
#include "raft_manager_callback.h"
#include "raft_state.h"
/**
 * @brief RaftManager 
 */
class RaftManager : public RaftManagerCallback {
 public:
  /**
  * @brief constructor
  * @param raft_parameters config of raft
  * @param member_ip_port address of any member from the cluster
  */
  RaftManager(std::shared_ptr<RaftParameters> raft_parameters);
  ~RaftManager();

  virtual bool ChangeState(std::string ip_port, int32_t term) override;
  virtual bool StartElection() override;

  virtual inline bool IsVoter() override { return election_->IsVoter(); }

  virtual std::pair<std::string, int32_t> GetLeader() override;

  void StartServer(std::string master_ip_port);

  friend class RaftServer;

 private:
  std::shared_ptr<Election> election_;

  std::shared_ptr<HeartbeatSensor> heartbeat_sensor_;
  std::shared_ptr<ApiImpl> api_impl_;

  //   std::shared_ptr<RaftParameters> raft_parameters_;
  //   std::shared_ptr<RaftState> raft_state_;
  //   std::shared_ptr<ClusterManager> cluster_manager_;
  //   std::shared_ptr<RPCCalls> rpc_calls_;
};