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

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
using namespace std;
std::string getLocalIP() {
  struct ifaddrs *interfaces = nullptr;
  struct ifaddrs *ifa = nullptr;
  char ip[INET_ADDRSTRLEN];
  if (getifaddrs(&interfaces) == -1) {
   return ""; // Error retrieving interfaces
  }
  // Loop through the list of interfaces
  for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
   // Check for IPv4 address and non-loopback interfaces
   if (ifa->ifa_addr->sa_family == AF_INET && 
    !(ifa->ifa_flags & IFF_LOOPBACK)) {
    struct sockaddr_in *sa_in = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
    inet_ntop(AF_INET, &(sa_in->sin_addr), ip, INET_ADDRSTRLEN);
    freeifaddrs(interfaces);
    return std::string(ip); // Return the first valid IP address
   }
  }
  freeifaddrs(interfaces);
  return ""; // No valid IP address found 
 }

void RunServer(
  int32_t election_timeout_low,
  int32_t election_timeout_high,
  int32_t heartbeat_timeout_,
  int32_t max_retries_,
  STATE state_, 
  std::string master_ip_port_,
  std::string server_address,
  std::string path,
  std::string cluster_key ) 
{
  std::string port=server_address;
  if(server_address=="null"){
    server_address="";
  }
  else {
    server_address=getLocalIP()+":"+port;
  }
  Store store(path);
  raftManager raft_manager(
                    election_timeout_low,
                    election_timeout_high,
                    heartbeat_timeout_,
                    server_address,
                    max_retries_,
                    master_ip_port_,
                    state_,
                    cluster_key);
  std::mutex raft_manager_mutex;
  auto log_store_ = std::make_shared<logStoreImpl>(raft_manager,raft_manager_mutex,store);
  log_store_->set_ptr(log_store_);
  raftServer<::log_request> raft_service(log_store_,raft_manager,raft_manager_mutex,cluster_key);
  Api_impl service(store,log_store_);
  

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.RegisterService(&raft_service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  cout<<(getLocalIP()+":"+port)<<"\n";
  server->Wait();
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
          stoi(argv[1]),stoi(argv[2]),
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

