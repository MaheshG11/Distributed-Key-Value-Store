#include "raft_dtypes.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include "grpcpp/grpcpp.h"

using namespace std;
STUB::STUB(const string& ip_port) {
  s1 = key_value_store_rpc::NewStub(
      grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
  s2 = raft::NewStub(
      grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
}

void RaftParameters::Print() {
  spdlog::info("max_retries {}", max_retries);
  spdlog::info("election_timeout_low {}", election_timeout_low.count());

  spdlog::info("election_timeout_high {}", election_timeout_high.count());

  spdlog::info("election_timeout {}", election_timeout.count());

  spdlog::info("heartbeat_timeout {}", heartbeat_timeout.count());

  spdlog::info("this_ip_port {}", this_ip_port);
  spdlog::info("cluster_key {}", cluster_key);
}