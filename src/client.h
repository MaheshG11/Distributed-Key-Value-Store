#ifndef CLIENT
#define CLIENT
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <utility>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"

class Client {
 private:
  std::unique_ptr<key_value_store_rpc::Stub> stub_;

 public:
  Client(std::string& ip_port);
  bool PUT(std::pair<std::string, std::string>& key_value);
  bool DELETE(std::string& key);
  std::pair<std::string, bool> GET(std::string& key);
};
#endif