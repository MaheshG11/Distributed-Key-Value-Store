#pragma once

#include <memory>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"

struct STUB {
  std::unique_ptr<KeyValueStoreRPC::Stub> s1;
  std::unique_ptr<Raft::Stub> s2;
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

struct NodeState {
  std::string ip_port;
  int64_t matchIndex;
  int64_t nextIndex;
};