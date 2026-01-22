//this will act as the abstraction layer between raft and server API
#pragma once
#ifndef log_store
#define log_store

#include <bits/stdc++.h>
#include "raft_manager.h"
template <typename T>
class logStore {
 public:
  logStore(raftManager& raft_manager, std::mutex& raft_manager_mutex);
  ~logStore();
  // void return type as it will be running asynchronously
  void append_entry(T request);
  void commit(int64_t entry_id_, bool commit);
  void stop_deque_thread();
  void start_deque_thread();
  void set_ptr(std::shared_ptr<logStore<T>> ptr);

 private:
  void start_();
  void start_candidate_deque();
  virtual void execute_entry(T& request) = 0;
  void append_entry_(T request);
  inline void process_dequed_entry(
      T& request, std::unique_lock<std::mutex>& raft_manager_lock);

 private:
  int64_t entry_id = 0;
  std::shared_ptr<logStore<T>> derieved_ptr = nullptr;
  raftManager& raft_manager_ptrraft_manager;
  std::queue<T> log_queue, candidate_queue;
  std::mutex log_queue_mutex, &raft_manager_mutex, candidate_queue_mutex;
  std::condition_variable log_queue_cv;
  std::thread deque_thread, candidate_queue_thread;
  std::atomic<bool> deque_thread_status, force_quit, candidate_deque_status;
};

#include "log_store.tpp"
#endif
