//This is incomplete
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include <grpcpp/grpcpp.h>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"

#include "raft_manager.h"

class RaftManagerTest : public ::testing::Test {
 protected:
  std::string s = "";
  void SetUp() override { raftManager raft_manager(300, 300, s, 3, s, LEADER); }
  void TearDown() override {}
};

TEST(RaftManagerTest, test_raft_manager) {
  std::string s = "";
  auto instance = std::make_unique<raftManager>(300, 300, s, 3, s, LEADER);
  ASSERT_NE(instance, nullptr);
}
