#include "raft_queue.h"
#include <spdlog/spdlog.h>
#include <memory>
// #include "store.cpp"
using namespace std;

/**
 * @brief constructor
 */
RaftQueue::RaftQueue(std::string& path) : store(path) {

  SPDLOG_INFO("RaftQueue::RaftQueue: Enter");
}
/**
  * @brief get All the entries from the given param to the end 
  * @param entry_id
  * @param request populates the entries in this request
  * @return true on success
  */
bool RaftQueue::GetEntries(int64_t entry_id, LogRequest& request) {
  SPDLOG_INFO("RaftQueue::GetEntries: Enter");

  auto* entries_field = request.mutable_entries();

  int64_t last;
  if ((*entries_field).size()) {
    last = (*entries_field)[(*entries_field).size() - 1].id();
  } else {
    last = GetMostRecentId();
  }
  SPDLOG_INFO("Entry id is {}", entry_id);

  while (entries_field->size() && last <= entry_id) {
    SPDLOG_INFO("Before remove size is {}", request.entries_size());
    entries_field->RemoveLast();
    SPDLOG_INFO("after remove size is {}", request.entries_size());
    if ((*entries_field).size()) {
      if ((*entries_field)[(*entries_field).size() - 1].id() < entry_id)
        last = (*entries_field)[(*entries_field).size() - 1].id();
      else if ((*entries_field)[(*entries_field).size() - 1].id() == entry_id) {
        entries_field->RemoveLast();
        break;
      }
    } else
      last = GetMostRecentId();
  }
  SPDLOG_INFO("last is set to {} ", last);
  lock_guard<mutex> lock1(log_entries_mtx_);

  auto [arr_idx, entry_idx] = find(last);
  SPDLOG_INFO("Got {} {}", arr_idx, entry_idx);

  if (entry_idx == -1)
    return false;
  SPDLOG_INFO("Size{}", log_entries_[arr_idx].size());
  SPDLOG_INFO("Before start size is {}", request.entries_size());
  while (true) {
    if (entry_idx >= 0 && entry_idx < log_entries_[arr_idx].size() &&
        entry_id <= log_entries_[arr_idx][entry_idx].id()) {
      StoreRequest entry = log_entries_[arr_idx][entry_idx--];
      SPDLOG_INFO("Adding entry_idx {} | Before size is {}", entry_idx + 1,
                  request.entries_size());
      entries_field->Add(move(entry));
      SPDLOG_INFO("After size is {}", request.entries_size());
    } else if (entry_idx < 0 and arr_idx > 0) {
      arr_idx--;
      entry_idx = log_entries_[arr_idx].size() - 1;
    } else
      break;
  }
  return true;
}

/**
  * @brief add entry to the queue 
  * @param entry
  */
bool RaftQueue::AppendEntry(StoreRequest& entry) {
  SPDLOG_INFO("RaftQueue::AppendEntry: Enter");

  lock_guard<mutex> lock1(log_entries_mtx_);
  if (log_entries_[in_use_log_entries_].size() >= 1e6) {
    if (!clearLog())
      return false;
  }
  log_entries_[in_use_log_entries_].emplace_back(entry);
  return true;
}

pair<string, bool> RaftQueue::GetValue(const StoreRequest& entry) {
  SPDLOG_INFO("RaftQueue::GetValue: Enter");

  return store.GET(entry.key());
}

/**
  * @brief add all entries in request 
  * @param request
  */
bool RaftQueue::AppendEntries(const LogRequest& request) {
  SPDLOG_INFO("RaftQueue::AppendEntries: Enter");

  auto entries = request.entries();
  int32_t id = GetMostRecentId(), req_id = entries.cbegin()->id();
  if (id < entries.cbegin()->id()) {
    for (auto entry : entries) {
      AppendEntry(entry);
    }
    return true;
  }
  auto idx = find(req_id);
  auto it = entries.begin();
  if (idx.first == -1)
    return false;
  while (it != entries.cend()) {

    if (it->id() != log_entries_[idx.first][idx.second].id()) {
      DropEntries(log_entries_[idx.first][idx.second].id());
      return false;
    }
    it++;
    advanceIdx(idx);
    if (idx.first >= 0)
      continue;
    break;
  }

  while (it != entries.cend()) {
    AppendEntry((*it));
    it++;
  }
  return true;
}

/**
  * @brief drop entries from entry id to the end 
  * @param entry_id
  */
bool RaftQueue::DropEntries(int64_t entry_id) {
  SPDLOG_INFO("RaftQueue::DropEntries: Enter");

  lock_guard<mutex> lock1(log_entries_mtx_);
  while ((*log_entries_[in_use_log_entries_].rbegin()).id() >= entry_id) {
    log_entries_[in_use_log_entries_].pop_back();
    if (!log_entries_[in_use_log_entries_].size()) {
      if (in_use_log_entries_ > 0)
        in_use_log_entries_--;
      else
        return false;
    }
  }
  return true;
}

/**
   * @brief commit entry till the given entry id
   * @param entry_id 
   */
bool RaftQueue::CommitEntry(int64_t entry_id) {
  SPDLOG_INFO("RaftQueue::CommitEntry: Enter");

  lock_guard<mutex> lock1(log_entries_mtx_);

  while (commit_id_ < entry_id) {
    ++commit_idx_;
    if (commit_idx_ >= log_entries_[commit_arr_id_].size()) {
      if (commit_arr_id_ < 2 && commit_idx_ >= 1e6) {
        commit_arr_id_++;
        commit_idx_ = 0;
      } else {
        commit_idx_--;
        return false;
      }
    }
    SPDLOG_INFO("Commiting index {}", commit_idx_);
    if (commit_idx_ < log_entries_[commit_arr_id_].size()) {
      if (entry_id >= log_entries_[commit_arr_id_][commit_idx_].id()) {
        SPDLOG_INFO("Commiting id {}",
                    log_entries_[commit_arr_id_][commit_idx_].id());

        if (execute(log_entries_[commit_arr_id_][commit_idx_])) {
          commit_id_ = log_entries_[commit_arr_id_][commit_idx_].id();
          continue;
        }

        return false;
      }
    }
  }
  return true;
}

/**
   * Returns most recent id 
   */
int64_t RaftQueue::GetMostRecentId() {
  SPDLOG_INFO("RaftQueue::GetMostRecentId: Enter");

  lock_guard<mutex> lock1(log_entries_mtx_);

  if (in_use_log_entries_ >= 0 and in_use_log_entries_ <= 2) {
    int32_t sz = log_entries_[in_use_log_entries_].size();
    if (sz) {
      SPDLOG_INFO("Id: {}", log_entries_[in_use_log_entries_][sz - 1].id());

      return log_entries_[in_use_log_entries_][sz - 1].id();
    }
  }
  return -1;
}

/**
  * @brief clears log, to be called when log is filled up
  */
bool RaftQueue::clearLog() {
  SPDLOG_INFO("RaftQueue::clearLog: Enter");

  if (in_use_log_entries_ < 2) {
    in_use_log_entries_++;
    return true;
  }
  lock_guard<mutex> lock1(log_entries_mtx_);
  swap(log_entries_[0], log_entries_[1]);
  swap(log_entries_[1], log_entries_[in_use_log_entries_]);
  log_entries_[in_use_log_entries_] = std::vector<StoreRequest>();
  return true;
}

/**
  * @brief Executes the entry 
  * @param entry 
  * @returns 
  */
bool RaftQueue::execute(StoreRequest& entry) {
  SPDLOG_INFO("RaftQueue::execute: Enter");

  if (entry.operation()) {
    auto pr = make_pair(entry.key(), entry.value());
    return store.PUT(pr);
  } else {
    string key = entry.key();
    return store.DELETE(key);
  }
}

/**
  * @brief Find entry next to given entry id  
  * @param entry_id 
  * @returns array index and index of the entry in that array
  */
pair<int, int> RaftQueue::find(int64_t entry_id) {
  SPDLOG_INFO("RaftQueue::find: Enter");
  entry_id++;
  int arr_idx = -1;
  if (log_entries_[in_use_log_entries_].size() &&
      log_entries_[in_use_log_entries_][0].id() <= entry_id) {
    arr_idx = in_use_log_entries_;
  } else if (in_use_log_entries_ - 1 >= 0 &&
             log_entries_[in_use_log_entries_ - 1][0].id() <= entry_id) {
    arr_idx = in_use_log_entries_ - 1;
  } else if (in_use_log_entries_ - 2 >= 0 &&
             log_entries_[in_use_log_entries_ - 2][0].id() <= entry_id) {
    arr_idx = in_use_log_entries_ - 2;
  } else
    return {arr_idx, arr_idx};

  int l = 0, r = log_entries_[arr_idx].size() - 1;
  int ans = -1;
  while (l <= r) {
    int mid = l + (r - l) / 2;
    if (log_entries_[arr_idx][mid].id() < entry_id) {
      l = mid + 1;
      ans = mid;

    } else if (log_entries_[arr_idx][mid].id() > entry_id) {
      r = mid - 1;
    } else {
      ans = mid;
      break;
    }
  }

  return {arr_idx, ans};
}

void RaftQueue::advanceIdx(std::pair<int, int>& idx) {
  SPDLOG_INFO("RaftQueue::advanceIdx: Enter");

  if (idx.first + 1 < 1e6 and log_entries_[idx.second].size() < idx.first + 1) {
    idx.first++;
    return;

  } else if (idx.second + 1 < 3 && log_entries_[idx.second + 1].size()) {
    idx.first = 0;
    idx.second++;
    return;
  }
  idx.first = -1;
  idx.second = -1;
}
