#include <grpcpp/grpcpp.h>

#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "api.h"
#include <string>

Api_impl::Api_impl(Store& store,
    std::shared_ptr<logStoreImpl> log_store_ptr):
    store(store),log_store_ptr(log_store_ptr){}

::grpc::Status Api_impl::get_rpc(::grpc::ServerContext* context, const ::store_request* request, ::store_response* response){
    std::string key=request->key();
    std::pair<std::string,bool> res=(store.GET(key));
    response->set_value(res.first);
    response->set_ok((bool)res.second);
    if(res.second){
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "key not found");

}

::grpc::Status Api_impl::delete_rpc(::grpc::ServerContext* context, const ::store_request* request, ::store_response* response){
    ::log_request request_;
    std::string key=request->key();
    request_.set_key(key);
    request_.set_request_type((int32_t)1);
    request_.set_entry_id((int32_t)-1);
    log_store_ptr->append_entry(request_);
    response->set_ok(true);
    return grpc::Status::OK;
}

::grpc::Status Api_impl::put_rpc(::grpc::ServerContext* context, const ::store_request* request, ::store_response* response){
    ::log_request request_;
    response->set_ok(true);
    request_.set_key(request->key());
    request_.set_value(request->value());
    request_.set_entry_id((int32_t)-1);

    request_.set_request_type((int32_t)0);
    log_store_ptr->append_entry(request_);
    
    return grpc::Status::OK;
}
