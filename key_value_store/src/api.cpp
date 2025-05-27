#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "store.h"
#include "api.h"
#include <string>

Api_impl::Api_impl(std::string &db_path){
    store=new Store(db_path);

}

::grpc::Status Api_impl::get_rpc(::grpc::ServerContext* context, const ::get_delete* request, ::get_response* response){
    std::string key=request->key();

    std::pair<std::string,bool> res=(store->GET(key));
    response->set_value(res.first);
    response->set_ok((bool)res.second);
    if(res.second){
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "key not found");

}

::grpc::Status Api_impl::delete_rpc(::grpc::ServerContext* context, const ::get_delete* request, ::put_delete_response* response){
    std::string key=request->key();
    bool res=(store->DELETE(key));
    response->set_ok(res);
    if(res){
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "key not found");
}

::grpc::Status Api_impl::put_rpc(::grpc::ServerContext* context, const ::put* request, ::put_delete_response* response){
    std::pair<std::string,std::string> key_value=std::make_pair(request->key(),request->value());
    bool res=(store->PUT(key_value));
    response->set_ok(res);
    if(res){
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "INTERNAL_SERVER_ERROR");
}
