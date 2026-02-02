#include "raft_manager.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <memory>
#include "kv_api/api.h"
#include "raft_dtypes.h"
#include "raft_server.h"
using namespace std;
/**
  * @brief constructor
  * @param raft_parameters config of raft
  * @param member_ip_port address of any member from the cluster
  */
RaftManager::RaftManager(shared_ptr<RaftParameters> raft_parameters) {
  SPDLOG_INFO("RaftManager(constructor): Enter");

  raft_parameters_ = raft_parameters;

  raft_state_ = make_shared<RaftState>();
  raft_state_->SetCommitIndex(-1);
  raft_state_->SetState(FOLLOWER);
  raft_state_->SetTerm(-1);
  raft_state_->SetLeaderAvailable(false);

  cluster_manager_ = make_shared<ClusterManager>(raft_parameters_, raft_state_);
  cluster_manager_->AddNode(raft_parameters_->this_ip_port);
  rpc_calls_ =
      make_shared<RPCCalls>(raft_parameters_, raft_state_, cluster_manager_);
  cluster_manager_->SetRPCCalls(rpc_calls_);

  election_ = make_shared<Election>(raft_state_, rpc_calls_);
}

RaftManager::~RaftManager() {
  SPDLOG_INFO("RaftManager::~RaftManager: Enter\n");
}

bool RaftManager::ChangeState(std::string ip_port, int32_t term_) {
  SPDLOG_INFO("RaftManager::ChangeState: Enter\n");

  cluster_manager_->UpdateLeader(ip_port, term_);

  return true;
}
bool RaftManager::StartElection() {
  SPDLOG_INFO("RaftManager::StartElection: Enter\n");

  promise<bool> election_prom;
  future<bool> election_fut = election_prom.get_future();
  election_->Start(move(election_prom));
  if (election_fut.get()) {
    ChangeState(raft_parameters_->this_ip_port, raft_state_->GetTerm() + 1);
    rpc_calls_->BroadcastNewLeader();

    return true;
  }
  return false;
}

std::pair<std::string, int32_t> RaftManager::GetLeader() {
  SPDLOG_INFO("RaftManager::GetLeader: Enter\n");

  return cluster_manager_->GetLeader();
}

void RaftManager::StartServer(std::string master_ip_port) {
  SPDLOG_INFO("RaftManager::StartServer");
  SPDLOG_WARN("RaftManager::StartServer ip port {}", master_ip_port);

  // get a shared_ptr to this manager (requires that the manager is
  // created as a shared_ptr)
  auto raft_manager_ptr = shared_from_this();

  // allocate service objects on the heap and keep them alive in members
  raft_server_impl_ = std::make_shared<RaftServer>(raft_manager_ptr);
  api_impl_ =
      std::make_shared<ApiImpl>(cluster_manager_->log_queue_, raft_state_);
  cluster_manager_->SetApiImpl(api_impl_);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(raft_parameters_->this_ip_port,
                           grpc::InsecureServerCredentials());
  builder.RegisterService(raft_server_impl_.get());
  builder.RegisterService(api_impl_.get());

  server_ = builder.BuildAndStart();
  SPDLOG_WARN("Server listening on {}", raft_parameters_->this_ip_port);

  if (!server_) {
    spdlog::error("server failed to start");
    throw std::runtime_error("unable to start server");
  }

  if (master_ip_port != "null") {
    auto status = (rpc_calls_->SendMemberRequest(master_ip_port, true));
  }

  heartbeat_sensor_ = make_shared<HeartbeatSensor>(this);
  heartbeat_sensor_->Start();
}