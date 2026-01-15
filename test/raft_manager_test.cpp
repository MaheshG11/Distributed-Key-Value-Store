//This is incomplete
#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <utility>
#include <cstdlib> 

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <chrono>
#include <thread>

#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"

#include "raft_manager.h"

class RaftManagerTest: public ::testing::Test{
protected:
    std::string s="";
    void SetUp() override{
        raftManager raft_manager(300,300,s,3,s,MASTER);
    }
    void TearDown() override{
    }
};


TEST(RaftManagerTest, test_raft_manager){
    std::string s="";
    auto instance = std::make_unique<raftManager>(300,300,s,3,s,MASTER);
    ASSERT_NE(instance,nullptr);
}
