#ifndef CLIENT
#define CLIENT 
#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include <memory>
#include <utility>
#include <string>

class Client{
private:
    std::unique_ptr<key_value_store_rpc::Stub> stub_;
public:
    Client(std::string &ip_port);
    bool PUT(std::pair<std::string,std::string> &key_value);
    bool DELETE(std::string &key);
    std::pair<std::string,bool> GET(std::string &key);

};
#endif