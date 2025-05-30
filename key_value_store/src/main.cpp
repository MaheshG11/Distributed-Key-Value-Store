#include <iostream>
#include "api.h"
#include <string>
#include "utility"
#include <memory>
using namespace std;

void RunServer(std::string server_address,std::string path ) {
  Api_impl service(server_address,path);

  service.Run();
}

int main(int argc, char* argv[]){
    RunServer(argv[1],argv[2]);
}