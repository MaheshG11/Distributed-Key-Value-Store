#include "rpc_calls.h"
#include <spdlog/spdlog.h>
#include <future>
#include <iostream>
#include <memory>
#include "raft_dtypes.h"
#include "raft_manager.h"
#include "raft_state.h"
using namespace std;
using namespace grpc;

/**
  * @brief constructor
  * @param cluster_manager_ shared pointer of ClusterManager
  * @param raft_parameters_ shared pointer of RaftParameters
  * @param raft_state_ shared pointer of RaftState
  */
RPCCalls::RPCCalls(shared_ptr<RaftParameters> raft_parameters,
                   shared_ptr<RaftState> raft_state,
                   shared_ptr<ClusterManager> cluster_manager)
    : cluster_manager_(cluster_manager),
      raft_parameters_(raft_parameters),
      raft_state_(raft_state),
      log_queue_(cluster_manager->log_queue_) {
  SPDLOG_INFO("RPCCalls(constructor): Enter");
}

/**
     * @brief broadcast log entry to all the other nodes
     * @param entry entry to broadcast 
     * @param success set value true if quorum agrees
     * @param success_fut future for success
     */
void RPCCalls::AppendLogEntries(atomic<int64_t>& commited_idx) {
  SPDLOG_INFO("RPCCalls::AppendLogEntries");

  if (raft_state_->GetState() != LEADER) {
    StopAppendentries();
    return;
  } else if (is_append_entries_running) {
    StopAppendentries();
  }
  std::thread new_thread = thread(
      [&](atomic<int64_t>& commited_idx) { appendLogEntries(commited_idx); },
      std::ref(commited_idx));
  swap(new_thread, append_entries_thread);
}

void RPCCalls::StopAppendentries() {
  SPDLOG_INFO("RPCCalls::StopAppendentries");

  is_append_entries_running = false;
  if (append_entries_thread.joinable())
    append_entries_thread.join();
}

/**
 * @brief forward log entry to master
 * @param entry the entry to forward to master
*/
bool RPCCalls::ForwardLogEntry(::LogRequest entry) {
  SPDLOG_INFO("RPCCalls::ForwardLogEntry: Enter");

  grpc::ClientContext context;
  ::LogResponse response;
  if (raft_state_->GetLeaderAvailable()) {
    cluster_manager_->GetLeaderStub()->SendLogEntry(&context, entry, &response);
  } else {
    return false;
  }
  return response.success();
}

/**
 * @brief broadcast new leader to all the other nodes
*/
void RPCCalls::BroadcastNewLeader() {
  SPDLOG_INFO("RPCCalls::BroadcastNewLeader: Enter");

  LeaderChangeRequest request;
  request.set_ip_port(raft_parameters_->this_ip_port);
  request.set_term(raft_state_->GetTerm());
  LeaderChangeResponse response;
  std::vector<std::future<void>> futures(cluster_manager_->GetNodesCnt() - 1);
  grpc::Status status;
  int cnt = 0, idx = 0;
  for (auto& [ip_port, stub] : (*cluster_manager_)) {
    if (ip_port == raft_parameters_->this_ip_port)
      continue;
    SPDLOG_WARN("Informing New Leader to {}", ip_port);

    int32_t idx_copy = idx++;
    futures[idx_copy] = std::async(
        std::launch::async,
        [&, idx_copy](NodeState& stub) {
          Retry(
              [&](grpc::ClientContext* ctx, const LeaderChangeRequest req,
                  LeaderChangeResponse* res) {
                return stub.s2->NewLeader(ctx, req, res);
              },
              raft_parameters_->heartbeat_timeout, (status), request,
              &(response));
          return;
        },
        ref(stub));
  }
  for (auto& i : futures)
    i.get();
}

/**
 * @brief broadcast if a member down or has joined the cluster
*/
bool RPCCalls::BroadcastMemberUpdate(MemberRequest request) {
  SPDLOG_INFO("RPCCalls::BroadcastMemberUpdate: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt() - 1;
  grpc::ClientContext context;
  std::vector<ClusterInfo> response(sz);
  std::vector<grpc::Status> status(sz);
  std::vector<std::future<void>> futures(sz);

  atomic<int> cnt{1};
  int idx = 0;

  for (auto& [ip_port, stub] : (*cluster_manager_)) {
    if (ip_port == raft_parameters_->this_ip_port)
      continue;
    SPDLOG_WARN("RPCCalls::BroadcastMemberUpdate:: ip_port:{} ", ip_port);

    int32_t idx_copy = idx++;
    futures[idx_copy] = std::async(
        std::launch::async,
        [&, idx_copy](NodeState& stub) {
          Retry(
              [&](grpc::ClientContext* ctx, const MemberRequest req,
                  ClusterInfo* res) {
                auto stat = stub.s2->UpdateClusterMember(ctx, req, res);
                if (stat.error_code() == grpc::StatusCode::OK)
                  cnt++;
                return stat;
              },
              raft_parameters_->heartbeat_timeout, (status[idx_copy]), request,
              &(response[idx_copy]));
          return;
        },
        ref(stub));
  }

  for (int32_t i = 0; i < idx; i++) {
    try {
      futures[i].get();
      if (!status[i].ok()) {
        continue;
      }
    } catch (const std::exception& e) {
      continue;
    }
  }

  if (cnt >= (cluster_manager_->GetNodesCnt() / 2)) {
    SPDLOG_WARN("RPCCalls::BroadcastMemberUpdate:: Returning True ");
    return true;
  }
  return false;
}

/**
 * @brief send heartbeat to the leader
*/
BeatsResponse RPCCalls::SendHeartbeat(HeartRequest& request) {
  SPDLOG_INFO("RPCCalls::SendHeartbeat: Enter");

  BeatsResponse response;
  response.set_is_leader(false);
  request.set_term(raft_state_->GetState());
  response.set_term(-1);
  grpc::ClientContext context;
  grpc::Status status;
  std::shared_ptr<Raft::Stub> leader_stub = cluster_manager_->GetLeaderStub();
  if (leader_stub != nullptr) {
    Retry(
        [&](grpc::ClientContext* ctx, const HeartRequest req,
            BeatsResponse* res) {
          return leader_stub->Heartbeat(ctx, req, res);
        },
        raft_parameters_->heartbeat_timeout, status, request, &response);
  }
  return response;
}

void RPCCalls::GetClusterInfo(ClusterInfo* request, std::string cluster_key_) {
  SPDLOG_INFO("RPCCalls::GetClusterInfo: Enter");
  int32_t sz = cluster_manager_->GetNodesCnt();
  request->set_cluster_key(cluster_key_);

  auto leader_info = cluster_manager_->GetLeader();
  if (!leader_info.first.empty()) {
    request->set_leader_ip_port(leader_info.first);
  }
  request->set_term(raft_state_->GetTerm());

  SPDLOG_WARN("RPCCalls::GetClusterInfo term:{} | {}", raft_state_->GetTerm(),
              leader_info.first);
  for (auto& [ip_port_it, stub] : (*cluster_manager_)) {

    request->add_ip_port(ip_port_it);
  }
}

void RPCCalls::CollectVotes(promise<bool> won, future<bool>& won_fut) {
  SPDLOG_INFO("RPCCalls::CollectVotes: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt() - 1;
  SPDLOG_INFO("RPCCalls::CollectVotes: GetNodesCnt{}", sz);

  std::vector<std::future<void>> futures(sz);
  std::vector<grpc::Status> status(sz);

  std::vector<VoteResponse> response(sz);
  VoteRequest request;
  request.set_ip_port(raft_parameters_->this_ip_port);
  request.set_term(raft_state_->GetTerm() + 1);
  mutex won_mtx;
  atomic<int32_t> votes{1};
  atomic<bool> got_res{false};
  int idx = 0;
  for (auto& [ip_port, stub] : (*cluster_manager_)) {
    SPDLOG_INFO("RPCCalls::CollectVotes: ip_port : {}", ip_port);
    if (ip_port == raft_parameters_->this_ip_port)
      continue;
    int32_t idx_copy = idx++;
    futures[idx_copy] = std::async(
        std::launch::async,
        [&, idx_copy](atomic<int32_t>& votes, NodeState& stub) {
          // retry starts here
          bool res = Retry(
              [&](grpc::ClientContext* ctx, const VoteRequest req,
                  VoteResponse* res) {
                auto status = stub.s2->VoteRPC(ctx, req, res);
                return status;
              },
              raft_parameters_->heartbeat_timeout, status[idx_copy], won_mtx,
              won, won_fut, votes, got_res, &response[idx_copy], request);

          // retry ends here
          if (res)
            return;
        },
        ref(votes), ref(stub));
  }
  for (auto& fut : futures)
    fut.get();
  if (won_fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
    SPDLOG_WARN("RPCCalls::CollectVotes : Votes{}", votes.load());
    return;
  }
  SPDLOG_WARN("RPCCalls::CollectVotes : Votes{}", votes.load());
  if (votes > cluster_manager_->GetNodesCnt() / 2) {
    won.set_value(true);
    return;
  }
  won.set_value(false);
}

template <typename Func, typename... Args>
bool RPCCalls::Retry(Func&& func, std::chrono::milliseconds& timeout,
                     grpc::Status& status, Args&&... args) {
  SPDLOG_INFO("RPCCalls::Retry 1: Enter");

  auto deadline = std::chrono::system_clock::now() +
                  std::chrono::milliseconds(100) + timeout;  // 500 ms timeout

  int retries = 0;
  while (retries < raft_parameters_->max_retries) {
    grpc::ClientContext context;
    deadline = chrono::system_clock::now() + chrono::milliseconds(100) +
               timeout;  // 500 ms timeout
    context.set_deadline(deadline);
    status = func(&context, forward<Args>(args)...);
    if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
      retries++;
    } else if (status.error_code() == grpc::StatusCode::UNAVAILABLE) {
      retries++;
      this_thread::sleep_for(timeout);
    } else {
      return true;
    }
  }
  spdlog::error("RPCCalls::Retry 1: failed request\n");

  return false;
}

template <typename Func, typename Response, typename Request>
bool RPCCalls::Retry(Func&& func, std::chrono::milliseconds& timeout,
                     grpc::Status& status, std::mutex& prom_mtx,
                     std::promise<bool>& prom, std::future<bool>& fut,
                     std::atomic<int32_t>& votes, std::atomic<bool>& got_res,
                     Response* response, Request& request) {
  SPDLOG_INFO("RPCCalls::Retry 2: Enter");

  auto deadline = std::chrono::system_clock::now() +
                  std::chrono::milliseconds(100) + timeout;  // 500 ms timeout

  int retries = 0;
  while (retries < raft_parameters_->max_retries) {
    grpc::ClientContext context;
    deadline = chrono::system_clock::now() + chrono::milliseconds(100) +
               timeout;  // 500 ms timeout
    context.set_deadline(deadline);
    SPDLOG_INFO("RPCCalls::Retry 2:{}{}", retries,
                raft_parameters_->max_retries);

    status = func(&context, request, response);
    if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
      retries++;
    } else if (status.error_code() == grpc::StatusCode::UNAVAILABLE) {
      retries++;
    } else if (response->success()) {
      votes++;
      if (got_res.load()) {
        return true;
      }
      lock_guard<mutex> lock1(prom_mtx);
      if (votes > (cluster_manager_->GetNodesCnt()) / 2 &&
          fut.wait_for(chrono::seconds(0)) != future_status::ready) {
        prom.set_value(true);
        got_res = true;
      }
      return true;
    } else {
      retries++;
    }
  }
  spdlog::error("RPCCalls::Retry 2: failed request\n");

  return false;
}

bool RPCCalls::SendMemberRequest(std::string ip_port, bool broadcast) {
  SPDLOG_INFO("RPCCalls::SendMemberRequest: Enter");

  auto stub = Raft::NewStub(
      grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
  grpc::ClientContext context;
  ClusterInfo response;
  MemberRequest request;
  request.set_ip_port(raft_parameters_->this_ip_port);
  request.set_cluster_key(raft_parameters_->cluster_key);
  request.set_broadcast(broadcast);

  auto status = stub->UpdateClusterMember(&context, request, &response);
  if (broadcast) {

    for (const auto& ip_port : response.ip_port()) {
      cluster_manager_->AddNode(ip_port);
    }
    string leader_ip_port = response.leader_ip_port();
    cluster_manager_->UpdateLeader(leader_ip_port, response.term());
  }
  if (status.error_code() == StatusCode::OK) {
    return true;
  }
  return false;
}

/**
     * @brief broadcast log entry to all the other nodes
     * @param entry entry to broadcast 
     * @param success set value true if quorum agrees
     * @param success_fut future for success
     */
void RPCCalls::appendLogEntries(atomic<int64_t>& commited_idx) {
  SPDLOG_INFO("RPCCalls::appendLogEntries: Enter");
  is_append_entries_running = true;
  // while (true) {
  while (cluster_manager_->Size() < raft_parameters_->size &&
         is_append_entries_running) {
    this_thread::sleep_for(chrono::milliseconds(500));
    SPDLOG_WARN("Waiting for all the nodes to join(initialize only): {} {}",
                cluster_manager_->Size(), raft_parameters_->size);
  }
  atomic<int64_t> commited_across_all = -1;
  int64_t mini_commit_id = LONG_MIN;

  while (is_append_entries_running) {
    SPDLOG_INFO("performing append entries iteration");

    vector<int64_t> match_idxs;

    auto it = cluster_manager_->begin();
    LogRequest request;
    request.set_term(raft_state_->GetTerm());
    int64_t min_commit_id = LONG_MAX;

    while (it != cluster_manager_->end()) {
      auto deadline = chrono::system_clock::now() + chrono::milliseconds(500);
      ClientContext context;
      context.set_deadline(deadline);

      if (it->second.commitedId == log_queue_->GetMostRecentId()) {
        match_idxs.push_back(it->second.matchIndex);
        min_commit_id = min(min_commit_id, it->second.commitedId);
        it++;
        continue;
      }
      it->second.nextIndex = log_queue_->GetNextId(it->second.matchIndex);
      log_queue_->GetEntries(it->second.nextIndex, request);
      request.set_commit_idx(commited_idx);
      SPDLOG_INFO("GetEntries Size: {}", request.entries_size());

      LogResponse response;
      auto& entries = request.entries();
      if (entries.size()) {
        if (it->first == raft_parameters_->this_ip_port) {
          response.set_success(log_queue_->AppendEntries(request));

        } else
          it->second.s2->SendLogEntry(&context, request, &response);
      } else {
        it++;
        min_commit_id = min(min_commit_id, it->second.commitedId);
        match_idxs.push_back(it->second.matchIndex);
        continue;
      }
      if (response.success()) {
        SPDLOG_INFO("Recieved success from {}", it->first);

        min_commit_id = min(min_commit_id, it->second.matchIndex);
        it->second.commitedId = it->second.matchIndex;
        if (entries.size()) {
          auto match_idx = entries.cbegin()->id();
          it->second.nextIndex = match_idx + 1;
          it->second.matchIndex = match_idx;
          match_idxs.push_back(match_idx);
        }
      } else {
        SPDLOG_WARN("Recieved failure from {}", it->first);

        it->second.nextIndex =
            max<int64_t>(1, it->second.nextIndex - 1);  // decrement next index
      }
      it++;
    }
    sort(match_idxs.begin(), match_idxs.end());
    int sz = match_idxs.size();
    if (sz) {

      commited_idx = match_idxs[(sz - 1) / 2];
      commited_idx.notify_all();
      SPDLOG_WARN("Matched indexes: {}", commited_idx.load());

      log_queue_->CommitEntry(commited_idx);
    }
    mini_commit_id = min_commit_id;
    if (commited_idx >= 0) {
      SPDLOG_WARN("Exiting");
      // exit(0);
    }
    if (mini_commit_id < log_queue_->GetMostRecentId())
      continue;
    // is_append_entries_running = false;
    this_thread::sleep_for(chrono::microseconds(100));
  }
}
