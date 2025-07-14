//this will act as the abstraction layer between raft and server API 
#pragma once
#ifndef log_store
#define log_store

#include "raft_manager.h"
#include <bits/stdc++.h>
template <typename T>
class logStore{
public:
    logStore(raftManager &raft_manager,
        std::unique_lock<std::mutex> &raft_manager_lock
    );
    ~logStore();

    // void return type as it will be running asynchronously
    void append_entry(T request);
    void commit(int64_t entry_id_, bool commit);
    void stop_deque_thread();
    void start_deque_thread();
private:
    int64_t entry_id=0;
    raftManager& raft_manager;
    std::queue<T> log_queue;
    std::mutex log_queue_mutex;
    std::condition_variable log_queue_cv;
    std::unique_lock<std::mutex> log_queue_lock,&raft_manager_lock;
    std::thread deque_thread;
    std::atomic<bool> deque_thread_status,force_quit;
    void start_();
    virtual void execute_entry(T &request) = 0;
    
};

#include "log_store.tpp"
#endif
