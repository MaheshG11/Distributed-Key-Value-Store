#pragma once
#include <chrono>
#include <future>
#include "raft_state.h"
#include "rpc_calls.h"

class Election {
 public:
  /**
  * @brief constructor
  */
  Election(std::shared_ptr<RaftState> raft_state_,
           std::shared_ptr<RPCCalls> rpc_calls_);

  Election() = delete;
  Election(const Election& other) = default;
  Election(Election&& other) = default;
  Election& operator=(Election& other) = default;
  Election& operator=(Election&& other) = default;

  /**
  * @brief Start election as a candidate
  */
  void Start(std::promise<bool> election_res);

  /**
   * @brief check if that this node can vote  
   * @param term_id with which to check if this node can vote
   */
  bool CanVote(int32_t term_id, int64_t last_commit_index,
               std::string& ip_port);

  inline bool IsVoter() { return last_voted_term_ > (raft_state_->GetTerm()); }

 private:
  std::shared_ptr<RaftState> raft_state_;
  std::shared_ptr<RPCCalls> rpc_calls_;
  int32_t last_voted_term_;
  std::string last_voted_;
  std::mutex last_voted_mtx_;
};