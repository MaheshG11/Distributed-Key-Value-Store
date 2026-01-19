#pragma once
#include <grpcpp/grpcpp.h>
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"
// #include "store.h"
#include <memory>
#include <string>
// #include "log_store_impl.cpp"

class Api_impl : public key_value_store_rpc ::Service {

 public:
  Api_impl();
  ::grpc::Status get_rpc(::grpc::ServerContext* context,
                         const ::store_request* request,
                         ::store_response* response);
  ::grpc::Status delete_rpc(::grpc::ServerContext* context,
                            const ::store_request* request,
                            ::store_response* response);
  ::grpc::Status put_rpc(::grpc::ServerContext* context,
                         const ::store_request* request,
                         ::store_response* response);

 private:
  //   Store& store;
  //   std::shared_ptr<logStoreImpl> log_store_ptr;
};
