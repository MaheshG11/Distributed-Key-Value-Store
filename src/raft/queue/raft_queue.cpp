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
  SPDLOG_INFO("Entry id is {}", entry_id);

  auto* entries_field = request.mutable_entries();

  int64_t last;
  int arr_idx, entry_idx;
  bool f = true;
  // remove unnecessary entries
  if (request.entries_size()) {
    last = (*entries_field)[request.entries_size() - 1].id();
    while (entries_field->size() && last <= entry_id) {
      SPDLOG_INFO("Before remove size is {}", request.entries_size());
      entries_field->RemoveLast();
      SPDLOG_INFO("after remove size is {}", request.entries_size());
      if (request.entries_size()) {
        last = (*entries_field)[request.entries_size() - 1].id();
      } else {
        last = GetMostRecentId();
        pair<int, int> pr = find(last);
        arr_idx = pr.first;
        entry_idx = pr.second;

        f = false;
      }
    }
  } else {
    last = GetMostRecentId();
    pair<int, int> pr = find(last);
    arr_idx = pr.first;
    entry_idx = pr.second;
    f = false;
  }
  if (f) {
    last--;
    pair<int, int> pr = find(last);
    arr_idx = pr.first;
    entry_idx = pr.second;
  }
  SPDLOG_INFO("last is set to {} ", last);
  lock_guard<mutex> lock1(log_entries_mtx_);

  // if (!request.entries_size()) {
  //   pair<int, int> pr = decreaseIdx({arr_idx, entry_idx});
  //   arr_idx = pr.first;
  //   entry_idx = pr.second;
  // }
  SPDLOG_INFO("Got {} {}", arr_idx, entry_idx);

  if (entry_idx == -1)
    return false;
  SPDLOG_INFO("Size{}", log_entries_[arr_idx].size());
  SPDLOG_INFO("Before start size is {}", request.entries_size());
  while (true) {
    if (entry_idx >= 0 && entry_id <= log_entries_[arr_idx][entry_idx].id()) {
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
bool RaftQueue::AppendEntry(const StoreRequest& entry) {
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

  const auto& entries = request.entries();
  if (entries.empty()) {
    return true;
  }
  int32_t id = GetMostRecentId(), req_id = entries.cbegin()->id();
  if (id < req_id) {
    for (const auto& entry : entries) {
      AppendEntry(entry);
    }
    return true;
  }

  pair<int, int> idx;
  {
    std::lock_guard<std::mutex> lock(log_entries_mtx_);
    idx = find(req_id);
  }
  auto it = entries.cbegin();
  if (idx.first == -1 and idx.second == -1)
    return false;
  while (it != entries.cend()) {

    if (it->id() != log_entries_[idx.first][idx.second].id()) {
      DropEntries(log_entries_[idx.first][idx.second].id());
      return false;
    }
    it++;
    advanceIdx(idx);
    if (idx.first >= 0 and idx.second >= 0)
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
int64_t RaftQueue::GetNextId(int64_t id) {
  lock_guard<mutex> lock1(log_entries_mtx_);

  if (id == -1) {
    if (log_entries_[0].size()) {
      id++;
      auto [arr_idx, entry_idx] = find(id);
      return log_entries_[arr_idx][entry_idx].id();
    }
  }
  auto [arr_idx, entry_idx] = find(id);

  if (arr_idx < 0 || entry_idx < 0) {
    return id;
  }
  if (entry_idx + 1 < log_entries_[arr_idx].size()) {
    return log_entries_[arr_idx][entry_idx + 1].id();
  }
  return id;
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
    SPDLOG_INFO("l {} | r {} | mid {} | ans{} | mid id {}", l, r, mid, ans,
                log_entries_[arr_idx][mid].id());
    if (log_entries_[arr_idx][mid].id() < entry_id) {
      ans = mid;
      l = mid + 1;
      SPDLOG_INFO("H1");

    } else if (log_entries_[arr_idx][mid].id() > entry_id) {
      r = mid - 1;
      SPDLOG_INFO("H2");

    } else {
      ans = mid;
      SPDLOG_INFO("H3");

      break;
    }
  }
  SPDLOG_INFO(" ans{}", ans);

  return {arr_idx, ans};
}

void RaftQueue::advanceIdx(std::pair<int, int>& idx) {
  SPDLOG_INFO("RaftQueue::advanceIdx: Enter");
  if (idx.first < 0 and idx.first > 2) {
    idx.first = -1;
    idx.second = -1;
    return;
  }
  if (idx.second + 1 < log_entries_[idx.first].size()) {
    idx.second++;
    return;
  } else if (idx.second >= 1e6 and idx.first < 2) {
    idx.first++;
    if (log_entries_[idx.first].size()) {
      idx.second = 0;
      return;
    } else {
      idx.second = -1;
      idx.first = -1;
      return;
    }
  }
  idx.first = -1;
  idx.second = -1;
}

std::pair<int, int> RaftQueue::decreaseIdx(std::pair<int, int> idx) {
  if (idx.second > 0) {
    idx.second--;
  } else if (idx.first > 0) {
    idx.first--;
    idx.second = log_entries_[idx.first].size() - 1;
  } else {
    idx.first = -1;
    idx.second = -1;
  }
  return {idx.first, idx.second};
}
