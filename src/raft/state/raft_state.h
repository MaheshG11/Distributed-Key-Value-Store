#pragma once
#include <spdlog/spdlog.h>
#include <iostream>
#include "raft_dtypes.h"

/**
 * @brief stores the current state of raft
 */
class RaftState {
 public:
  inline RaftState() { spdlog::info("RaftState(constructor): Enter"); };
  /**
  * @brief Get Current role of this node 
  */
  inline STATE GetState() {
    spdlog::info("RaftState::GetState: Enter");
    return state_;
  }

  /**
  * @brief Get Current role of this node 
  * @param state set the state to
  */
  inline bool SetState(STATE state) {
    spdlog::info("RaftState::SetState: Enter");
    state_ = state;
    return true;
  }

  /**
   * @brief Get current term 
   */
  inline int32_t GetTerm() {
    spdlog::info("RaftState::GetTerm: Enter");
    return term_.load();
  }

  /**
   * @brief set term
   * @param term new term
   * 
   */
  inline void SetTerm(int32_t term) {
    spdlog::info("RaftState::SetTerm: Enter");
    term_ = term;
  }

  /**
   * @brief get current commit index
   */
  inline int64_t GetCommitIndex() {
    spdlog::info("RaftState::GetCommitIndex: Enter");
    return commit_index_.load();
  }

  /**
   * @brief set commit index
   */
  inline void SetCommitIndex(int64_t commit_index) {
    spdlog::info("RaftState::SetCommitIndex: Enter");
    commit_index_ = commit_index;
  }

  /**
   *  @brief Sets leader available variable
   */
  inline void SetLeaderAvailable(bool is_leader_available) {
    spdlog::info("RaftState::SetLeaderAvailable: Enter");
    is_leader_available_.store(is_leader_available);
  }

  /**
   *  @brief Get leader available variable
   */
  inline bool GetLeaderAvailable() {
    spdlog::info("RaftState::SetLeaderAvailable: Enter");
    return is_leader_available_.load();
  }

 private:
  std::atomic<int32_t> term_;
  std::atomic<bool> is_leader_available_;
  std::atomic<int64_t> commit_index_;

  std::atomic<STATE> state_;
};