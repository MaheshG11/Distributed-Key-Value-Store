#pragma once
#ifndef RaftManager
#define RaftManager

#include <bits/stdc++.h>
#include <grpcpp/grpcpp.h>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"
#include <mutex>
#include <chrono>
#include <future>
#include <atomic>
#include <condition_variable>
#include <stdexcept>
// #include "args.h"


struct STUB{
    std::unique_ptr<key_value_store_rpc::Stub> s1;
    std::unique_ptr<raft::Stub> s2;
    STUB(std::string &ip_port);
};

enum STATE{
    MASTER,
    CANDIDATE,
    FOLLOWER
};

class raftManager{
public:
    int32_t term_id=0,max_retries;
    STATE state;   
    std::chrono::milliseconds election_timeout,heartbeat_timeout;
    std::chrono::time_point<std::chrono::system_clock> last_contact,last_voted;
    std::string this_ip_port;
    std::map<std::string,STUB> mpp;

    raftManager(int32_t election_timeout_,
                                    int32_t heartbeat_timeout_,
                                    std::string &this_ip_port_,
                                    int32_t max_retries_,
                                    std::string master_ip_port_,
                                    STATE state_
                                );
    ~raftManager(){};

    void add_node_to_cluster(std::string ip_port);

    
    template <typename T>
    bool broadcast_log_entry(T &request);
    bool broadcast_commit(int64_t entry_id,bool commit = false);

    bool broadcast_member_update(::member_request request);


    // std::chrono::time_point<std::chrono::system_clock> get_last_contact();
    void start_heartbeat_sensing();
    void start_voting();
    void broadcast_new_master();
    
    inline void stop_heartbeat_sensing();
    inline STATE get_state();
    inline bool change_state_to(STATE state_, std::string ip_port,int32_t term_);
    inline std::pair<std::string,int32_t> get_master();

    inline void update_last_contact();
    inline void update_last_voted();
    inline bool can_vote(int32_t term_id_);
    

private:

    
    std::mutex master_info_mutex,
                last_contact_mutex,
                last_voted_mutex,
                state_mutex,
                heartbeat_mutex;
    std::string master_ip_port="";
    std::unique_ptr<raft::Stub> master_stub;
    
    std::atomic<bool> run_heartbeat_sensing = false,is_running=false;
    std::condition_variable heartbeat_cv;

    void share_cluster_info_with(std::string ip_port,std::string cluster_key);
    inline void wait_for(std::chrono::milliseconds &timeout);
    inline int32_t get_nodes_cnt();
    
    template <typename Func, typename... Args>
    auto retry(Func&& func,std::chrono::milliseconds &timeout ,Args&&... args)
            -> decltype(func(std::forward<Args>(args)...));

    inline void update_master(std::string ip_port,int32_t term_);
    
};
#include "raft_manager.tpp"
#endif