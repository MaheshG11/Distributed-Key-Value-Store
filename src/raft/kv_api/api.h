#pragma once
#include <grpcpp/grpcpp.h>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"
// #include "store.h"
#include <memory>
#include <string>
// #include "log_store_impl.cpp"

class Api_impl : public KeyValueStoreRPC::Service {

 public:
  Api_impl();
  grpc::Status WriteRPC(grpc::ServerContext* context,
                        const StoreRequest* request, StoreResponse* response);
  grpc::Status GetRPC(grpc::ServerContext* context, const StoreRequest* request,
                      StoreResponse* response);

 private:
  //   Store& store;
  //   std::shared_ptr<logStoreImpl> log_store_ptr;
};
