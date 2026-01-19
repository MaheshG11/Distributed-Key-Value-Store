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
void RPCCalls::BroadcastLogEntry(::log_request& entry, promise<bool> success,
                                 future<bool>& success_fut) {
  spdlog::info("RPCCalls::BroadcastLogEntry: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt();
  entry.set_term(raft_state_->GetTerm());
  grpc::ClientContext context;
  std::vector<::log_response> response(sz);
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
              [&](grpc::ClientContext* ctx, const log_request req,
                  log_response* res) {
                auto status = stub.s2->send_log_entry(ctx, req, res);
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
  ::commit_request request;
  request.set_entry_id(entry_id);
  request.set_commit(commit);
  request.set_term(raft_state_->GetTerm());
  grpc::ClientContext context;
  std::vector<commit_response> response(sz);
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
              [&](grpc::ClientContext* ctx, const commit_request req,
                  commit_response* res) {
                auto status = stub.s2->commit_log_entry(ctx, req, res);
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
bool RPCCalls::ForwardLogEntry(::log_request entry) {
  spdlog::info("RPCCalls::ForwardLogEntry: Enter");

  grpc::ClientContext context;
  ::log_response response;
  {
    lock_guard<mutex> lock1(cluster_manager_->GetLeaderMutex());
    cluster_manager_->GetLeaderStub()->send_log_entry(&context, entry,
                                                      &response);
  }
  return response.success();
}

/**
 * @brief broadcast new leader to all the other nodes
*/
void RPCCalls::BroadcastNewLeader() {
  spdlog::info("RPCCalls::BroadcastNewLeader: Enter");

  ::leader_change_request request;
  request.set_ip_port(raft_parameters_->this_ip_port);
  request.set_term(raft_state_->GetTerm());
  ::leader_change_response response;
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
              [&](grpc::ClientContext* ctx, const leader_change_request req,
                  leader_change_response* res) {
                return stub.s2->new_leader(ctx, req, res);
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
bool RPCCalls::BroadcastMemberUpdate(::member_request request) {
  spdlog::info("RPCCalls::BroadcastMemberUpdate: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt() - 1;
  grpc::ClientContext context;
  std::vector<::member_response> response(sz);
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
              [&](grpc::ClientContext* ctx, const member_request req,
                  member_response* res) {
                auto stat = stub.s2->update_cluster_member(ctx, req, res);
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
beats_response RPCCalls::SendHeartbeat(heart_request& request) {
  spdlog::info("RPCCalls::SendHeartbeat: Enter");

  ::beats_response response;
  response.set_is_leader(false);
  request.set_term(raft_state_->GetState());
  response.set_term(-1);
  grpc::ClientContext context;
  grpc::Status status;
  std::unique_lock<std::mutex> leader_info_lock(
      cluster_manager_->GetLeaderMutex());
  std::unique_ptr<raft::Stub>& leader_stub = cluster_manager_->GetLeaderStub();
  if (leader_stub != nullptr) {
    Retry(
        [&](grpc::ClientContext* ctx, const heart_request req,
            beats_response* res) {
          return leader_stub->heart_beat(ctx, req, res);
        },
        raft_parameters_->heartbeat_timeout, status, request, &response);
  }
  return response;
}

bool RPCCalls::ShareClusterInfo(std::string ip_port, std::string cluster_key_) {
  spdlog::info("RPCCalls::ShareClusterInfo: Enter");

  int32_t sz = cluster_manager_->GetNodesCnt();
  ::cluster_info request;
  ::commit_response response;
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
  auto res = (it->second).s2->share_cluster_info(&context, request, &response);
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

  std::vector<::vote_response> response(sz);
  ::vote_request request;
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
              [&](grpc::ClientContext* ctx, const vote_request req,
                  vote_response* res) {
                auto status = stub.s2->vote_rpc(ctx, req, res);
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

bool RPCCalls::SendMemberRequest(std::string ip_port, bool broadcast,
                                 bool to_drop) {
  spdlog::info("RPCCalls::SendMemberRequest: Enter");

  auto stub = raft::NewStub(
      grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
  grpc::ClientContext context;
  member_response response;
  member_request request;
  request.set_ip_port(raft_parameters_->this_ip_port);
  request.set_cluster_key(raft_parameters_->cluster_key);
  request.set_to_drop(to_drop);
  request.set_broadcast(broadcast);

  auto status = stub->update_cluster_member(&context, request, &response);
  if (status.error_code() == StatusCode::OK && response.success()) {
    return true;
  }
  return false;
}
