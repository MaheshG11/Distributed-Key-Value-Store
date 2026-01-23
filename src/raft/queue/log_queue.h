#pragma once
#include <grpcpp/grpcpp.h>
#include <map>
#include <queue>
#include <thread>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"
class RaftQueue {
 public:
  /**
   * @brief constructor
   */
  RaftQueue();

  /**
  * @brief get All the entries from the given param to the end 
  * or 128 whichever is less
  * @param entry_id
  * @param request populates the entries in this request
  * @return true on success
  */
  bool GetEntries(int64_t entry_id, LogRequest& request);

  /**
  * @brief add entry to the queue 
  * @param entry
  */
  bool AppendEntry(const StoreRequest& entry);

  /**
  * @brief add all entries in request 
  * @param request
  */
  bool AppendEntries(const LogRequest& request);

  /**
  * @brief drop entries from entry id to the end 
  * @param entry_id
  */
  bool DropEntries(int64_t entry_id);

  /**
   * @brief commit entry till the given entry id
   */
  bool CommitEntry(int64_t entry_id);

  /**
   * Returns most recent id 
   */
  int64_t GetMostRecentId();

 private:
  /**
  * @brief clears log, to be called when log is filled up
  */
  bool clearLog();

  /**
  * @brief Executes the entry 
  * @param entry 
  * @returns 
  */
  bool execute(StoreRequest& entry);

  /**
  * @brief Find entry with given entry id  
  * @param entry_id 
  * @returns array index and index of the entry in that array
  */
  std::pair<int, int> find(int64_t entry_id);

  /**
   * @brief Initializes append entries thread
   */
  void appendEntries();

  void advanceIdx(std::pair<int, int>& idx);

 private:
  std::mutex log_entries_mtx_;
  std::vector<StoreRequest> log_entries_[3];
  int in_use_log_entries_ = 0;
  int64_t commit_idx_ = -1;
  int32_t commit_id_, commit_arr_id_ = 0;
  std::thread append_entries_thread_;
};