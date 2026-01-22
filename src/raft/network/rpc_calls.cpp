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
      raft_state_(raft_state) {
  spdlog::info("RPCCalls(constructor): Enter");
}

/**
 * @brief broadcast log entry to all the other nodes
 * @param entry entry to broadcast
 * @param success set value true if quorum agrees
 * @param success_fut future for success
*/
void RPCCalls::BroadcastLogEntry(::LogRequest& entry, promise<bool> success,
                                 future<bool>& success_fut) {
  spdlog::info("RPCCalls::BroadcastLogEntry: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt();
  entry.set_term(raft_state_->GetTerm());
  grpc::ClientContext context;
  std::vector<LogResponse> response(sz);
  std::vector<grpc::Status> status(sz);
  std::vector<std::future<void>> futures(sz);
  std::mutex success_mtx;
  atomic<bool> got_res{false};
  atomic<int32_t> votes{1};
  int cnt = 0, idx = 0;
  for (auto& [ip_port, stub] : (*cluster_manager_)) {
    if (ip_port == raft_parameters_->this_ip_port)
      continue;
    int32_t idx_copy = idx++;
    // async starts here

    futures[idx_copy] = std::async(
        std::launch::async,
        [&](atomic<int32_t>& votes, STUB& stub) {
          // retry starts here
          bool res = Retry(
              [&](grpc::ClientContext* ctx, const LogRequest req,
                  LogResponse* res) {
                auto status = stub.s2->SendLogEntry(ctx, req, res);
                return status;
              },
              raft_parameters_->heartbeat_timeout, status[idx_copy],
              success_mtx, success, success_fut, votes, got_res,
              &response[idx_copy], entry);

          // retry ends here
          if (res)
            return;
        },
        ref(votes), ref(stub));
    //async ends here
  }

  for (auto& fut : futures)
    fut.get();
  if (success_fut.wait_for(std::chrono::seconds(0)) ==
      std::future_status::ready) {
    return;
  }
  success.set_value(false);
}

/**
 * @brief broadcast commit with entry id
 * @param entry_id commit entry with this id
 * @param commit if true commit the entry else drop it
 * @param success set value true if quorum agrees
 * @param success_fut future for success
*/
void RPCCalls::BroadcastCommit(int64_t entry_id, bool commit,
                               promise<bool> success,
                               future<bool>& success_fut) {
  spdlog::info("RPCCalls::BroadcastCommit: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt() - 1;
  ::CommitRequest request;
  request.set_entry_id(entry_id);
  request.set_commit(commit);
  request.set_term(raft_state_->GetTerm());
  grpc::ClientContext context;
  std::vector<CommitResponse> response(sz);
  std::vector<grpc::Status> status(sz);
  std::vector<std::future<void>> futures(sz);
  std::mutex success_mtx;
  atomic<bool> got_res{false};
  atomic<int32_t> votes{1};
  int cnt = 0, idx = 0;
  for (auto& [ip_port, stub] : (*cluster_manager_)) {
    if (ip_port == raft_parameters_->this_ip_port)
      continue;
    int32_t idx_copy = idx++;
    // async starts here

    futures[idx_copy] = std::async(
        std::launch::async,
        [&, idx_copy](atomic<int32_t>& votes, STUB& stub) {
          // retry starts here
          bool res = Retry(
              [&](grpc::ClientContext* ctx, const CommitRequest req,
                  CommitResponse* res) {
                auto status = stub.s2->CommitLogEntry(ctx, req, res);
                return status;
              },
              raft_parameters_->heartbeat_timeout, status[idx_copy],
              success_mtx, success, success_fut, votes, got_res,
              &response[idx_copy], request);

          // retry ends here
          if (res)
            return;
        },
        ref(votes), ref(stub));
    //async ends here
  }

  for (auto& fut : futures)
    fut.get();
  if (success_fut.wait_for(std::chrono::seconds(0)) ==
      std::future_status::ready) {
    return;
  }
  success.set_value(false);
}

/**
 * @brief forward log entry to master
 * @param entry the entry to forward to master
*/
bool RPCCalls::ForwardLogEntry(::LogRequest entry) {
  spdlog::info("RPCCalls::ForwardLogEntry: Enter");

  grpc::ClientContext context;
  ::LogResponse response;
  if (raft_state_->GetLeaderAvailable()) {
    lock_guard<mutex> lock1(cluster_manager_->GetLeaderMutex());
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
  spdlog::info("RPCCalls::BroadcastNewLeader: Enter");

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
    spdlog::warn("Informing New Leader to {}", ip_port);

    int32_t idx_copy = idx++;
    futures[idx_copy] = std::async(
        std::launch::async,
        [&, idx_copy](STUB& stub) {
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
  spdlog::info("RPCCalls::BroadcastMemberUpdate: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt() - 1;
  grpc::ClientContext context;
  std::vector<MemberResponse> response(sz);
  std::vector<grpc::Status> status(sz);
  std::vector<std::future<void>> futures(sz);

  atomic<int> cnt{1};
  int idx = 0;

  for (auto& [ip_port, stub] : (*cluster_manager_)) {
    if (ip_port == raft_parameters_->this_ip_port)
      continue;
    int32_t idx_copy = idx++;
    futures[idx_copy] = std::async(
        std::launch::async,
        [&, idx_copy](STUB& stub) {
          Retry(
              [&](grpc::ClientContext* ctx, const MemberRequest req,
                  MemberResponse* res) {
                auto stat = stub.s2->UpdateClusterMember(ctx, req, res);
                if (stat.error_code() == grpc::StatusCode::OK && res->success())
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
      if (!status[i].ok() || !response[i].success()) {
        continue;
      }
    } catch (const std::exception& e) {
      continue;
    }
  }

  if (cnt >= (cluster_manager_->GetNodesCnt() / 2)) {
    return true;
  }
  return false;
}

/**
 * @brief send heartbeat to the leader
*/
BeatsResponse RPCCalls::SendHeartbeat(HeartRequest& request) {
  spdlog::info("RPCCalls::SendHeartbeat: Enter");

  BeatsResponse response;
  response.set_is_leader(false);
  request.set_term(raft_state_->GetState());
  response.set_term(-1);
  grpc::ClientContext context;
  grpc::Status status;
  std::unique_lock<std::mutex> leader_info_lock(
      cluster_manager_->GetLeaderMutex());
  std::unique_ptr<Raft::Stub>& leader_stub = cluster_manager_->GetLeaderStub();
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

bool RPCCalls::ShareClusterInfo(std::string ip_port, std::string cluster_key_) {
  spdlog::info("RPCCalls::ShareClusterInfo: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt();
  ClusterInfo request;
  CommitResponse response;
  request.set_cluster_key(cluster_key_);
  grpc::ClientContext context;
  request.set_leader_ip_port((cluster_manager_->GetLeader()).first);
  request.set_term(raft_state_->GetTerm());

  grpc::Status status;
  spdlog::warn("RPCCalls::ShareClusterInfo term:{} | {}",
               raft_state_->GetTerm(), (cluster_manager_->GetLeader()).first);
  for (auto& [ip_port_it, stub] : (*cluster_manager_)) {
    request.add_ip_port(ip_port_it);
  }
  auto it = cluster_manager_->Find(ip_port);
  if (it == cluster_manager_->end()) {
    return false;
  }
  auto res = (it->second).s2->ShareClusterInfo(&context, request, &response);
  spdlog::warn("RPCCalls::ShareClusterInfo response:{} ", res.error_code());

  if (res.error_code() == grpc::StatusCode::OK) {
    return true;
  }
  return false;
}

void RPCCalls::CollectVotes(promise<bool> won, future<bool>& won_fut) {
  spdlog::info("RPCCalls::CollectVotes: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt() - 1;
  spdlog::info("RPCCalls::CollectVotes: GetNodesCnt{}", sz);

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
    spdlog::info("RPCCalls::CollectVotes: ip_port : {}", ip_port);
    if (ip_port == raft_parameters_->this_ip_port)
      continue;
    int32_t idx_copy = idx++;
    futures[idx_copy] = std::async(
        std::launch::async,
        [&, idx_copy](atomic<int32_t>& votes, STUB& stub) {
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
    spdlog::warn("RPCCalls::CollectVotes : Votes{}", votes.load());
    return;
  }
  spdlog::warn("RPCCalls::CollectVotes : Votes{}", votes.load());
  if (votes > cluster_manager_->GetNodesCnt() / 2) {
    won.set_value(true);
    return;
  }
  won.set_value(false);
}

template <typename Func, typename... Args>
bool RPCCalls::Retry(Func&& func, std::chrono::milliseconds& timeout,
                     grpc::Status& status, Args&&... args) {
  spdlog::info("RPCCalls::Retry 1: Enter\n");

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
  spdlog::info("RPCCalls::Retry 2: Enter\n");

  auto deadline = std::chrono::system_clock::now() +
                  std::chrono::milliseconds(100) + timeout;  // 500 ms timeout

  int retries = 0;
  while (retries < raft_parameters_->max_retries) {
    grpc::ClientContext context;
    deadline = chrono::system_clock::now() + chrono::milliseconds(100) +
               timeout;  // 500 ms timeout
    context.set_deadline(deadline);
    spdlog::info("RPCCalls::Retry 2:{}{}", retries,
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
  spdlog::info("RPCCalls::SendMemberRequest: Enter");

  auto stub = Raft::NewStub(
      grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
  grpc::ClientContext context;
  MemberResponse response;
  MemberRequest request;
  request.set_ip_port(raft_parameters_->this_ip_port);
  request.set_cluster_key(raft_parameters_->cluster_key);
  request.set_broadcast(broadcast);

  auto status = stub->UpdateClusterMember(&context, request, &response);
  if (status.error_code() == StatusCode::OK && response.success()) {
    return true;
  }
  return false;
}
