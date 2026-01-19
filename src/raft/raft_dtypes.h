#pragma once

#include <memory>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"

struct STUB {
  std::unique_ptr<key_value_store_rpc::Stub> s1;
  std::unique_ptr<raft::Stub> s2;
  STUB(const std::string& ip_port);
};

enum STATE { LEADER, CANDIDATE, FOLLOWER };

struct RaftParameters {
  int32_t max_retries;
  std::chrono::milliseconds election_timeout_low;
  std::chrono::milliseconds election_timeout_high;
  std::chrono::milliseconds election_timeout;
  std::chrono::milliseconds heartbeat_timeout;
  std::string this_ip_port;
  std::string cluster_key;

  void Print();
};