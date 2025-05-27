#ifndef API
#define API
#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "store.h"
#include <string>
class Api_impl : public key_value_store_rpc :: Service{

private:
    int port;
    Store* store;
public:
    Api_impl(std::string &db_path);
    ::grpc::Status get_rpc(::grpc::ServerContext* context, const ::get_delete* request, ::get_response* response);
    ::grpc::Status delete_rpc(::grpc::ServerContext* context, const ::get_delete* request, ::put_delete_response* response);
    ::grpc::Status put_rpc(::grpc::ServerContext* context, const ::put* request, ::put_delete_response* response);
 
};

#endif 