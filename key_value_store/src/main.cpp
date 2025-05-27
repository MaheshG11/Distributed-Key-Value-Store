#include <iostream>
#include "api.h"
#include <string>
#include "utility"
#include <memory>
using namespace std;

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  std::string path("/project/data");
  Api_impl service(path);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);project

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(){
    RunServer();
}