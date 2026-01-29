#include <iostream>
#include <memory>
#include <random>
#include <string>
#include "raft_manager.h"
#include "raft_server.h"
// #include "store.h"
#include "utility"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
using namespace std;
const string CLUSTER_KEY = "ABC";

int get_random(int low, int high) {
  boost::random::random_device rd;
  boost::random::uniform_int_distribution<int> dist(low, high);
  int x = dist(rd);  //
  return x;
}

std::string getLocalIP() {
  struct ifaddrs* interfaces = nullptr;
  struct ifaddrs* ifa = nullptr;
  char ip[INET_ADDRSTRLEN];
  if (getifaddrs(&interfaces) == -1) {
    return "";  // Error retrieving interfaces
  }
  // Loop through the list of interfaces
  for (ifa = interfaces; ifa != nullptr; ifa = ifa->ifa_next) {
    // Check for IPv4 address and non-loopback interfaces
    if (ifa->ifa_addr->sa_family == AF_INET &&
        !(ifa->ifa_flags & IFF_LOOPBACK)) {
      struct sockaddr_in* sa_in =
          reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
      inet_ntop(AF_INET, &(sa_in->sin_addr), ip, INET_ADDRSTRLEN);
      freeifaddrs(interfaces);
      return std::string(ip);  // Return the first valid IP address
    }
  }
  freeifaddrs(interfaces);
  return "";  // No valid IP address found
}

void RunServer(int32_t election_timeout_low, int32_t election_timeout_high,
               int32_t heartbeat_timeout, int32_t max_retries,
               std::string master_ip_port, std::string path) {
  RaftParameters raft_parameters;
  raft_parameters.cluster_key = CLUSTER_KEY;
  raft_parameters.election_timeout = std::chrono::milliseconds(
      get_random(election_timeout_low, election_timeout_high));
  raft_parameters.election_timeout_high =
      std::chrono::milliseconds(election_timeout_high);
  raft_parameters.election_timeout_low =
      std::chrono::milliseconds(election_timeout_low);
  raft_parameters.heartbeat_timeout =
      std::chrono::milliseconds(heartbeat_timeout);
  raft_parameters.max_retries = max_retries;
  raft_parameters.this_ip_port = getLocalIP() + ":5556";
  raft_parameters.path = path;
  raft_parameters.size = 5;

  shared_ptr<RaftParameters> raft_param_ptr =
      make_shared<RaftParameters>(raft_parameters);
  raft_parameters.Print();

  RaftManager raft_manager(raft_param_ptr);

  raft_manager.StartServer(master_ip_port);
}

void get_help() {

  cout << "\nEnter the details in the following manner\n";
  cout << "\ndistributed_kv_store <election low> <election high> <heartbeat "
          "timeout> <max retries> <member_ip_port> <path>\n";
  cout << "\n\n\n";
  cout << "election timeout low and high (in ms): Election timeout will be "
          "generated at random between these numbers\n";
  cout << "heartbeat timeout (in ms):          Server will check if leader is "
          "alive every heartbeat timeout \n";
  cout << "max retries :                       How many times will this node "
          "retry when communicating with others\n\n";
  cout << "master_ip_port :                    IP Port of one of the members\n";
  cout << "path :                              Path at which the k-v store is "
          "to be initialized \n\n";
}

int main(int argc, char* argv[]) {

  auto console = spdlog::stdout_color_mt("console");
  spdlog::set_default_logger(console);
  spdlog::set_level(spdlog::level::warn);
  // spdlog::info("func='{}'", spdlog::source_loc::c
  spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");

  spdlog::set_level(spdlog::level::warn);

  // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
  spdlog::info("Logger initialized");

  try {
    if (string(argv[1]) == "--help") {
      get_help();

    } else {

      RunServer(stoi(argv[1]), stoi(argv[2]), stoi(argv[3]), stoi(argv[4]),
                argv[5], argv[6]);
    }
  } catch (const std::exception& e) {
    cerr << "Caught exception: " << e.what() << "\n\n";
    cout << "Seems you had some errors while starting the program";
    get_help();
  }
}
