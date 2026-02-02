#include "raft_dtypes.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include "grpcpp/grpcpp.h"

using namespace std;
NodeState::NodeState(const string& ip_port) {
  s1 = KeyValueStoreRPC::NewStub(
      grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
  s2 = Raft::NewStub(
      grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
  matchIndex = -1;
  nextIndex = -1;
  commitedId = -1;
}

void RaftParameters::Print() {
  SPDLOG_INFO("max_retries {}", max_retries);

  SPDLOG_INFO("election_timeout_low {}", election_timeout_low.count());
  SPDLOG_INFO("election_timeout_high {}", election_timeout_high.count());

  SPDLOG_INFO("election_timeout {}", election_timeout.count());
  SPDLOG_INFO("heartbeat_timeout {}", heartbeat_timeout.count());

  SPDLOG_INFO("this_ip_port {}", this_ip_port);
  SPDLOG_INFO("cluster_key {}", cluster_key);
}