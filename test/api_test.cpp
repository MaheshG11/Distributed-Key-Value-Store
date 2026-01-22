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

#include "api.h"
#include "client.h"

static std::string a = "a", b = "b", c = "c", d = "d";
class ClientTest : public ::testing::Test {
 protected:
  std::string dirName = std::filesystem::temp_directory_path() / "test_dir",
              ip_port = "0.0.0.0:50051";
  pid_t pid = -1;

  Client* client;
  void SetUp() override {
    std::filesystem::create_directory(dirName);
    pid = fork();
    if (pid == 0) {
      execl("/project/build/temp", "temp", ip_port.c_str(), dirName.c_str(),
            NULL);
    }
    // stopping this for bit of time so that the new process starts
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    client = new Client(ip_port);
    std::pair<std::string, std::string> key_value = {a, b};
    client->PUT(key_value);
    key_value = {b, c};
    client->PUT(key_value);
  }

  // TearDown() runs after each test (optional)
  void TearDown() override {
    // cleanup code if needed
    delete client;
    kill(pid, SIGKILL);
    std::filesystem::remove_all(dirName);
    pid = -1;
  }
};
TEST_F(ClientTest, test_put) {
  std::pair<std::string, std::string> p1 = {c, d};
  bool v = client->PUT(p1);
  EXPECT_EQ(v, true);
  std::pair<std::string, bool> p = client->GET(c);
  EXPECT_EQ(p.second, true);
  EXPECT_EQ(p.first, d);
}
TEST_F(ClientTest, test_get) {
  std::pair<std::string, bool> p = client->GET(c);
  EXPECT_EQ(p.second, false);
  EXPECT_EQ(p.first, std::string(""));
  p = client->GET(a);
  EXPECT_EQ(p.second, true);
  EXPECT_EQ(p.first, b);
}
TEST_F(ClientTest, test_delete) {
  std::pair<std::string, bool> p = client->GET(a);
  EXPECT_EQ(p.second, true);
  EXPECT_EQ(p.first, b);
  client->DELETE(a);
  p = client->GET(a);
  EXPECT_EQ(p.second, false);
  EXPECT_EQ(p.first, std::string(""));
}