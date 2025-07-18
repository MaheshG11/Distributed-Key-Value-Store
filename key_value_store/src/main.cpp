#include <iostream>
#include "api.h"
#include <string>
#include "utility"
#include <memory>
#include "raft/raft_manager.h"
#include "log_store_impl.cpp"
#include "raft/raft_server.h"
#include "store.h"
#include <random>

using namespace std;

void RunServer(
  int32_t election_timeout_,
  int32_t heartbeat_timeout_,
  int32_t max_retries_,
  STATE state_, 
  std::string master_ip_port_,
  std::string server_address,
  std::string path,
  std::string cluster_key ) 
{
  if(server_address=="null"){
    server_address="";
  }
  else server_address="0.0.0.0:"+server_address;
  Store store(path);
  raftManager raft_manager(
                    election_timeout_,
                    heartbeat_timeout_,
                    server_address,
                    max_retries_,
                    master_ip_port_,
                    state_,
                    cluster_key);
  // cout<<"here\n";
  std::mutex raft_manager_mutex;
  std::unique_lock<std::mutex> raft_manager_lock(raft_manager_mutex,std::defer_lock);
  auto log_store_ = std::make_shared<logStoreImpl>(raft_manager,raft_manager_lock,store);
  cout<<"now here\n";
  // raftServer<::log_request> raft_service(log_store_,raft_manager,raft_manager_lock,cluster_key);
  // Api_impl service(store,log_store_);
  

  // grpc::ServerBuilder builder;
  // builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // builder.RegisterService(&service);

  // std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  // std::cout << "Server listening on " << server_address << std::endl;

  // server->Wait();
}


void get_help(){

  cout<<"\nEnter the details in the following manner\n";
  cout<<"\ndistributed_kv_store <election low> <election high> <heartbeat timeout> <max retries> <state> <leader ip port> <port> <path> <cluster key>\n";
  cout<<"\n\n\n";
  cout<<"election timeout low, high (in ms): Election timeout will be generated at random between these numbers\n";
  cout<<"heartbeat timeout (in ms):          Server will check if leader is alive every heartbeat timeout \n";
  cout<<"max retries :                       How many times will this node retry when communicating with others\n\n";
  cout<<"state :                             Enter if the starting node is leader or follower by l and f respectively \n";
  cout<<"leader ip port :                    At what ip-port leader is running write \"null\" if the state you set is l \n\n";
  cout<<"port :                              What is the port current application should run on\n";
  cout<<"path :                              Path at which the k-v store is to be initialized \n\n";
  cout<<"cluster key :                       Key which is reponsible for accessing the cluster \n\n";;
}

int get_random(int min, int max) {
    if(max<min)throw std::runtime_error("high is less then low");
    std::random_device rd;  // non-deterministic generator
    std::mt19937 gen(rd()); // seed the Mersenne Twister RNG
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}


int main(int argc, char* argv[]){
    // RunServer(argv[1],argv[2]);
    try{
      if(string(argv[1])=="--help" ){
        get_help();
 
      }
      else{
        
        STATE state_= ((std::string("l")==argv[5])? MASTER : 
                     (std::string("f")==argv[5])? FOLLOWER : CANDIDATE);
        if(state_==CANDIDATE){
          throw std::runtime_error("incorrect state assignment");
        }
        RunServer(
          get_random(stoi(argv[1]),stoi(argv[2])),
          stoi(argv[3]),
          stoi(argv[4]),
          state_, 
          argv[6],
          argv[7],
          argv[8],
          argv[9]); 
      }
    } catch (const std::exception& e) {
      cerr << "Caught exception: " << e.what() << "\n\n";
      cout<<"Seems you had some errors while starting the program";
      get_help();

    }

}