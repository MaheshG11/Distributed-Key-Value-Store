#include <iostream>
#include "api.h"
#include <string>
#include "utility"
#include <memory>
#include "raft/raft_manager.h"
#include "log_store_impl.cpp"
#include "raft/raft_server.h"
#include "store.h"

using namespace std;

void RunServer(
  int32_t election_timeout_,
  int32_t heartbeat_timeout_,
  int32_t max_retries_,
  std::string master_ip_port_,
  STATE state_, 
  std::string server_address,
  std::string path,
  std::string cluster_key ) 
{
  Store store(path);
  
  raftManager raft_manager(
                    election_timeout_,
                    heartbeat_timeout_,
                    server_address,
                    max_retries_,
                    master_ip_port_,
                    state_);
  std::mutex raft_manager_mutex;
  std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex,std::defer_lock);
  auto log_store_ = std::make_shared<logStoreImpl>(raft_manager,raft_manager_lock,store);
  raftServer<::log_request> raft_service(log_store_,raft_manager,raft_manager_lock,cluster_key);
  Api_impl service(store,log_store_);
  

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char* argv[]){
    // RunServer(argv[1],argv[2]);
}