#pragma once
#include <string>
#include "raft/log_store.h"
#include "raft/raft_manager.h"
#include "store.h"

class logStoreImpl : public logStore<::LogRequest> {
 public:
  logStoreImpl(raftManager& raft_manager, std::mutex& raft_manager_mutex,
               Store& store)
      : logStore<::LogRequest>(raft_manager, raft_manager_mutex),
        store(store) {}

  void execute_entry(::LogRequest& request) override {
    if (request.request_type() == 0) {
      std::pair<std::string, std::string> kv_pair = {request.key(),
                                                     request.value()};
      store.PUT(kv_pair);
    } else if (request.request_type() == 1) {
      std::string key = request.key();
      store.DELETE(key);
    }
  }

 private:
  Store& store;
};