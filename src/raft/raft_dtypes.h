#pragma once

#include <memory>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"

struct NodeState {
  std::shared_ptr<KeyValueStoreRPC::Stub> s1;
  std::shared_ptr<Raft::Stub> s2;
  std::string ip_port;
  int64_t matchIndex;
  int64_t nextIndex;
  int64_t commitedId;

  NodeState(const std::string& ip_port);
};

enum STATE { LEADER, CANDIDATE, FOLLOWER };

struct RaftParameters {
  int32_t max_retries;
  int64_t size;
  std::chrono::milliseconds election_timeout_low;
  std::chrono::milliseconds election_timeout_high;
  std::chrono::milliseconds election_timeout;
  std::chrono::milliseconds heartbeat_timeout;
  std::string this_ip_port;
  std::string cluster_key;
  std::string path;  // path at which data will be stored

  void Print();
};
