#pragma once
#include <memory>
#include "cluster_manager.h"
#include "raft_dtypes.h"
#include "raft_state.h"
#include "rpc_calls.h"

class RaftManagerCallback {
 public:
  virtual ~RaftManagerCallback() = default;

  virtual bool ChangeState(std::string ip_port, int32_t term) = 0;
  virtual bool StartElection() = 0;
  virtual bool IsVoter() = 0;
  virtual std::pair<std::string, int32_t> GetLeader() = 0;

  friend class HeartbeatSensor;
  friend class RaftServer;

 protected:
  std::shared_ptr<RaftParameters> raft_parameters_;
  std::shared_ptr<RaftState> raft_state_;
  std::shared_ptr<ClusterManager> cluster_manager_;
  std::shared_ptr<RPCCalls> rpc_calls_;
};