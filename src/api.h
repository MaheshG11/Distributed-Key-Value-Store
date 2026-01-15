#pragma once
#ifndef API
#define API
#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "store.h"
#include <string>
#include <memory>
#include "log_store_impl.cpp"

class Api_impl : public key_value_store_rpc :: Service{

private:
    Store& store;
    std::shared_ptr<logStoreImpl> log_store_ptr;
public:
    Api_impl(Store& store,std::shared_ptr<logStoreImpl> log_store_ptr);
    ::grpc::Status get_rpc(::grpc::ServerContext* context, const ::store_request* request, ::store_response* response);
    ::grpc::Status delete_rpc(::grpc::ServerContext* context, const ::store_request* request, ::store_response* response);
    ::grpc::Status put_rpc(::grpc::ServerContext* context, const ::store_request* request, ::store_response* response);
};

#endif