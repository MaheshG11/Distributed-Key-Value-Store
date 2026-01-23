#include "log_queue.h"
#include <memory>
using namespace std;

/**
 * @brief constructor
 */
RaftQueue::RaftQueue() {}
/**
  * @brief get All the entries from the given param to the end 
  * or 128 whichever is less
  * @param entry_id
  * @param request populates the entries in this request
  * @return true on success
  */
bool RaftQueue::GetEntries(int64_t entry_id, LogRequest& request) {
  lock_guard<mutex> lock1(log_entries_mtx_);
  auto* entries_field = request.mutable_entries();
  int64_t last = (*entries_field)[(*entries_field).size() - 1].id();
  while (entries_field->size() && last >= entry_id) {
    entries_field->RemoveLast();
    last = (*entries_field)[(*entries_field).size() - 1].id();
  }
  auto [arr_idx, entry_idx] = find(last);
  if (arr_idx == -1)
    return false;
  while (entry_idx < log_entries_[arr_idx].size()) {
    if (entry_idx >= 0 && entry_id <= log_entries_[arr_idx][entry_idx].id()) {
      StoreRequest entry = log_entries_[arr_idx][entry_idx--];
      entries_field->Add(move(entry));
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
  lock_guard<mutex> lock1(log_entries_mtx_);
  if (log_entries_[in_use_log_entries_].size() >= 1e6) {
    if (!clearLog())
      return false;
  }
  log_entries_[in_use_log_entries_].push_back(entry);
  return true;
}

/**
  * @brief add all entries in request 
  * @param request
  */
bool RaftQueue::AppendEntries(const LogRequest& request) {
  auto entries = request.entries();
  int32_t id = GetMostRecentId(), req_id = entries.cbegin()->id();
  if (id < entries.cbegin()->id()) {
    for (auto entry : entries) {
      AppendEntry(entry);
    }
    return true;
  }
  auto idx = find(req_id);
  auto it = entries.cbegin();
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
  lock_guard<mutex> lock1(log_entries_mtx_);

  while (commit_id_ <= entry_id) {
    ++commit_idx_;
    if (commit_idx_ >= log_entries_[commit_arr_id_].size()) {
      if (commit_arr_id_ < 2) {
        commit_arr_id_++;
        commit_idx_ = 0;
      } else {
        return false;
      }
    }
    if (commit_idx_ < log_entries_[commit_arr_id_].size()) {
      if (entry_id <= log_entries_[commit_arr_id_][commit_idx_].id()) {
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
  lock_guard<mutex> lock1(log_entries_mtx_);

  if (in_use_log_entries_ >= 0 and in_use_log_entries_ <= 2) {
    int32_t sz = log_entries_[in_use_log_entries_].size();
    return log_entries_[in_use_log_entries_][sz - 1].id();
  }
  return -1;
}

/**
  * @brief clears log, to be called when log is filled up
  */
bool RaftQueue::clearLog() {

  if (in_use_log_entries_ < 2) {
    in_use_log_entries_++;
    return true;
  }
  lock_guard<mutex> lock1(log_entries_mtx_);
  swap(log_entries_[0], log_entries_[1]);
  swap(log_entries_[1], log_entries_[in_use_log_entries_]);
  log_entries_[in_use_log_entries_] = std::vector<StoreRequest>();
}

/**
  * @brief Executes the entry 
  * @param entry 
  * @returns 
  */
bool RaftQueue::execute(StoreRequest& entry) {
  //TODO
}

/**
  * @brief Find entry with given entry id  
  * @param entry_id 
  * @returns array index and index of the entry in that array
  */
pair<int, int> RaftQueue::find(int64_t entry_id) {

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
  while (l <= r) {
    int mid = l + (r - l) / 2;
    if (log_entries_[arr_idx][mid].id() == entry_id)
      return {arr_idx, mid};
    else if (log_entries_[arr_idx][mid].id() < entry_id) {
      l = mid + 1;
    } else {
      r = mid - 1;
    }
  }
  return {-1, -1};
}

void RaftQueue::advanceIdx(std::pair<int, int>& idx) {
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
