
template <typename T>
logStore<T>::logStore(raftManager& raft_manager, std::mutex& raft_manager_mutex)
    : raft_manager(raft_manager), raft_manager_mutex(raft_manager_mutex) {
  force_quit = false;
  deque_thread_status = false;
  candidate_deque_status = false;
  std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
  if (raft_manager.get_state() == LEADER) {
    raft_manager_lock.unlock();
    start_deque_thread();
  }
}

template <typename T>
logStore<T>::~logStore() {
  stop_deque_thread();
  if (candidate_queue_thread.joinable()) {
    candidate_queue_thread.join();
  }
}

template <typename T>
void logStore<T>::append_entry(T request) {
  std::async(
      std::launch::async, [&](T request) { this->append_entry_(request); },
      request);
}

template <typename T>
void logStore<T>::append_entry_(T request) {
  STATE state_ = raft_manager.get_state();
  if (state_ == FOLLOWER && request.entry_id() == -1) {
    std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex);
    raft_manager.forward_log_entry(request);
    return;
  }
  if (state_ == CANDIDATE) {
    std::unique_lock<std::mutex> lock(candidate_queue_mutex);
    candidate_queue.push(request);
    if (candidate_deque_status == false) {
      candidate_queue_thread =
          std::thread(&logStore::start_candidate_deque, this);
    }
    return;
  }
  std::unique_lock<std::mutex> log_queue_lock(log_queue_mutex);
  log_queue.push(request);
  log_queue_lock.unlock();
  log_queue_cv.notify_one();
}

template <typename T>
void logStore<T>::start_deque_thread() {
  force_quit = true;
  std::unique_lock<std::mutex> log_queue_lock(log_queue_mutex);
  log_queue_cv.wait(log_queue_lock,
                    [this]() { return deque_thread_status == false; });
  deque_thread = std::thread(&logStore::start_, this);
}

template <typename T>
void logStore<T>::stop_deque_thread() {
  deque_thread_status = false;
  force_quit = true;
  log_queue_cv.notify_one();
  if (deque_thread.joinable()) {
    deque_thread.join();
  }
}

template <typename T>
void logStore<T>::set_ptr(std::shared_ptr<logStore<T>> ptr) {
  derieved_ptr = ptr;
}

template <typename T>
void logStore<T>::commit(int64_t entry_id_, bool commit) {
  std::unique_lock<std::mutex> log_queue_lock(log_queue_mutex);
  if (!commit) {
    if (log_queue.size()) {
      log_queue.pop();
    }
    return;
  }

  if (log_queue.size()) {
    T request = log_queue.front();
    log_queue.pop();
    log_queue_lock.unlock();
    if (request.entry_id() != entry_id_)
      return;
    derieved_ptr->execute_entry(request);
  }
}

template <typename T>
void logStore<T>::start_() {
  deque_thread_status = true;
  force_quit = false;
  std::unique_lock<std::mutex> log_queue_lock(log_queue_mutex, std::defer_lock),
      raft_manager_lock(raft_manager_mutex, std::defer_lock);

  while (deque_thread_status) {
    log_queue_lock.lock();
    log_queue_cv.wait(log_queue_lock,
                      [this]() { return log_queue.size() > 0 || force_quit; });
    log_queue_lock.unlock();
    raft_manager_lock.lock();
    if (force_quit || raft_manager.get_state() != LEADER) {
      raft_manager_lock.unlock();
      break;
    } else {
      raft_manager_lock.unlock();
    }
    log_queue_lock.lock();
    T request = log_queue.front();
    log_queue.pop();
    log_queue_lock.unlock();
    process_dequed_entry(request, raft_manager_lock);
  }
  deque_thread_status = false;
  log_queue_cv.notify_one();
}

template <typename T>
void logStore<T>::start_candidate_deque() {
  candidate_deque_status = true;
  while (true) {
    if (raft_manager.get_state() == CANDIDATE) {
      std::this_thread::sleep_for(raft_manager.election_timeout);
      continue;
    }
    break;
  }

  std::unique_lock<std::mutex> lock(candidate_queue_mutex);
  while (candidate_queue.size()) {

    raft_manager.forward_log_entry(candidate_queue.front());
    candidate_queue.pop();
  }
  candidate_deque_status = false;
}

template <typename T>
inline void logStore<T>::process_dequed_entry(
    T& request, std::unique_lock<std::mutex>& raft_manager_lock) {
  raft_manager_lock.lock();
  bool is_commited = true;

  if (raft_manager.get_state() == LEADER) {
    is_commited = false;
    if (request.entry_id() == -1) {
      request.set_entry_id(entry_id);
      if (raft_manager.broadcast_log_entry(request)) {
        if (raft_manager.broadcast_commit(entry_id, true)) {
          entry_id++;
          is_commited = true;
        }
      } else if (is_commited == false) {
        raft_manager.broadcast_commit(entry_id, is_commited);
      }
    }
  }

  if (is_commited) {
    derieved_ptr->execute_entry(request);
  }
}
