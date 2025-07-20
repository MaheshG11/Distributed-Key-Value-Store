#pragma once
#include "raft/log_store.h"
#include "store.h"
#include <string>
#include "raft/raft_manager.h"

class logStoreImpl : public logStore<::log_request>{
public:
    logStoreImpl(raftManager &raft_manager,
        std::mutex &raft_manager_mutex,
        Store &store
    ):logStore<::log_request>(raft_manager,raft_manager_mutex),store(store){}

    void execute_entry(::log_request &request) override {
        if(request.request_type()==0){
            std::pair<std::string,std::string> kv_pair={request.key(),request.value()};
            store.PUT(kv_pair);
        }
        else if(request.request_type()==1){
            std::string key=request.key();
            store.DELETE(key);
        }
    }

private:
    Store& store;
};