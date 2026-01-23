#include "heartbeat_sensor.h"
#include <spdlog/spdlog.h>
#include <atomic>
#include <chrono>
#include <thread>

using namespace std;

HeartbeatSensor::HeartbeatSensor(RaftManagerCallback* callback)
    : rpc_calls_(callback->rpc_calls_),
      raft_state_(callback->raft_state_),
      raft_parameters_(callback->raft_parameters_),
      cluster_manager_(callback->cluster_manager_),
      callback_(callback),
      sense_({false}) {
  UpdateLastContact();
  spdlog::info("HeartbeatSensor(constructor): Enter");
}

/**
   * @brief begin hearbeat sensing
   */
void HeartbeatSensor::Start() {
  spdlog::info("HeartbeatSensor::Start: Enter");
  start();
}

/**
   * @brief stop hearbeat sensing
   */
void HeartbeatSensor::Stop() {
  spdlog::info("HeartbeatSensor::Stop: Enter");

  sense_ = false;
  if (heartbeat_thread_.joinable()) {
    heartbeat_thread_.join();
  }
}

/**
   * @brief begin hearbeat sensing
   */
void HeartbeatSensor::start() {
  spdlog::info("HeartbeatSensor::start: Enter");

  HeartRequest request;
  request.set_term(raft_state_->GetTerm());
  BeatsResponse response;
  sense_ = true;
  while (sense_.load()) {
    spdlog::info("HeartbeatSensor::sending heatbeart");

    if (raft_state_->GetState() == LEADER) {
      spdlog::info("I am the leader");
      std::this_thread::sleep_for(raft_parameters_->election_timeout_high);
      continue;
    }
    response = rpc_calls_->SendHeartbeat(request);
    if (response.is_leader()) {
      spdlog::info("I reached the leader");

      UpdateLastContact();
      std::this_thread::sleep_for(raft_parameters_->heartbeat_timeout);
      continue;
    } else if (response.term() > (raft_state_->GetTerm())) {
      try {
        cluster_manager_->UpdateLeader(response.leader_ip_port(),
                                       response.term());
        rpc_calls_->AppendLogEntries(api_impl_->commited_idx);

        continue;
      } catch (const std::exception& e) {
        spdlog::info(" heatbeart error {}", e.what());
      }
    }
    raft_state_->SetLeaderAvailable(false);
    std::this_thread::sleep_for(raft_parameters_->election_timeout);
    if ((callback_->IsVoter()))
      continue;
    bool res = callback_->StartElection();
    if (res) {
      spdlog::info("Election won by me:{}",
                   (cluster_manager_->GetLeader().first));

    } else {
      spdlog::info("I lost the election");
    }
  }
}