#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <utility>

#include "raft_queue.h"

static std::string a = "a", b = "b", c = "c", d = "d";
class RaftQueueTest : public ::testing::Test {
 protected:
  RaftQueue* raft_queue;
  std::string dirName = std::filesystem::temp_directory_path() / "test_dir";
  void SetUp() override {
    std::filesystem::create_directory(dirName);
    raft_queue = new RaftQueue(dirName);
    StoreRequest entry;
    std::pair<std::string, std::string> p1 = {"a", "b"}, p2 = {"b", "c"};
    entry.set_key(p1.first);
    entry.set_value(p1.second);
    entry.set_id(0);
    entry.set_operation(1);
    raft_queue->AppendEntry(entry);
    entry.set_key(p2.first);
    entry.set_value(p2.second);
    entry.set_id(1);
    entry.set_operation(1);
    raft_queue->AppendEntry(entry);
    entry.set_key(p1.first);
    entry.set_value(p1.second);
    entry.set_id(2);
    entry.set_operation(1);
    raft_queue->AppendEntry(entry);
    entry.set_key(p2.first);
    entry.set_value(p2.second);
    entry.set_id(3);
    entry.set_operation(1);
    raft_queue->AppendEntry(entry);
  }

  // TearDown() runs after each test (optional)
  void TearDown() override {
    // cleanup code if needed
    delete raft_queue;
    std::filesystem::remove_all(dirName);
  }
};
TEST_F(RaftQueueTest, test_AppendEntry) {
  std::pair<std::string, std::string> p1 = {"c", "d"};
  StoreRequest entry;
  entry.set_key(p1.first);
  entry.set_value(p1.second);
  entry.set_id(4);
  entry.set_operation(1);
  raft_queue->AppendEntry(entry);

  EXPECT_EQ(raft_queue->GetCurrentSize(), 5);
}
TEST_F(RaftQueueTest, test_GetValue_CommitEntry) {
  raft_queue->CommitEntry(0);
  StoreRequest entry;
  entry.set_key("a");
  EXPECT_EQ(raft_queue->GetValue(entry).first, b);
  entry.set_key("b");
  EXPECT_EQ(raft_queue->GetValue(entry).second, false);
  raft_queue->CommitEntry(1);
  entry.set_key("b");
  EXPECT_EQ(raft_queue->GetValue(entry).first, c);
}
TEST_F(RaftQueueTest, test_GetEntries) {
  LogRequest request;
  EXPECT_EQ(request.entries_size(), 0);
  raft_queue->GetEntries(0, request);
  EXPECT_EQ(request.entries_size(), 4);
  raft_queue->GetEntries(1, request);
  EXPECT_EQ(request.entries_size(), 3);
  raft_queue->GetEntries(2, request);
  EXPECT_EQ(request.entries_size(), 2);
  raft_queue->GetEntries(1, request);
  EXPECT_EQ(request.entries_size(), 3);
  raft_queue->GetEntries(2, request);
  EXPECT_EQ(request.entries_size(), 2);
  raft_queue->GetEntries(0, request);
  EXPECT_EQ(request.entries_size(), 4);
}