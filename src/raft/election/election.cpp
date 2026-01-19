#include "election.h"
#include <spdlog/spdlog.h>
#include <atomic>
#include <future>
using namespace std;

Election::Election(shared_ptr<RaftState> raft_state,
                   shared_ptr<RPCCalls> rpc_calls)
    : raft_state_(raft_state), rpc_calls_(rpc_calls) {
  spdlog::info("Election(constructor): Enter");

  last_voted_ = "";
  last_voted_term_ = -1;
}
void Election::Start(promise<bool> election_res) {
  spdlog::info("Election::Start: Enter");

  promise<bool> won_prom;
  future<bool> won = won_prom.get_future();
  raft_state_->SetState(CANDIDATE);
  auto fut = async(std::launch::async, [&]() {
    return rpc_calls_->CollectVotes(move(won_prom), won);
  });

  if (won.get()) {  // Election Won
    raft_state_->SetTerm(raft_state_->GetTerm() + 1);
    election_res.set_value(true);
    return;
  } else {  // Election Lost
    election_res.set_value(false);
  }
}

bool Election::CanVote(int32_t term_id, int64_t last_commit_index,
                       string& ip_port) {
  spdlog::info("Election::CanVote: Enter");

  lock_guard<mutex> lock1(last_voted_mtx_);
  if (raft_state_->GetTerm() < term_id) {
    last_voted_ = ip_port;
    last_voted_term_ = term_id;
    return true;
  }
  if (last_voted_ != "" && last_voted_ != ip_port) {
    return false;
  }

  return ((term_id == raft_state_->GetTerm()) &&
          last_commit_index >= raft_state_->GetCommitIndex());
}
