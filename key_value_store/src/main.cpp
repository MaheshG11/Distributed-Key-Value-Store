#include <iostream>
#include "api.h"
#include <string>
#include "utility"
#include <memory>
using namespace std;

void RunServer(std::string server_address,std::string path ) {
  Api_impl service(path);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char* argv[]){
    RunServer(argv[1],argv[2]);
}