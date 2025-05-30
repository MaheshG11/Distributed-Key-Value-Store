#include <grpcpp/grpcpp.h>

#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "store.h"
#include "api.h"
#include <string>
#include <memory>
#include <utility>

Store* store;
Api_impl::Api_impl(std::string &server_address,std::string &path)
:server_address(server_address){
    store=new Store(path);
}
void Api_impl::Run(){
    ::grpc::ServerBuilder builder;

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);
    cq_ = builder.AddCompletionQueue();
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

        // Spawn one handler for each RPC
    new put_rpc_CallData(&service_, cq_.get());
    new delete_rpc_CallData(&service_, cq_.get());
    new get_rpc_CallData(&service_, cq_.get());

    HandleRpcs();
}

Api_impl::get_rpc_CallData::get_rpc_CallData(key_value_store_rpc::AsyncService* service, ::grpc::ServerCompletionQueue* cq)
    :service_(service),cq_(cq),responder_(&ctx_),status_(CREATE){
        Proceed();
}
Api_impl::delete_rpc_CallData::delete_rpc_CallData(key_value_store_rpc::AsyncService* service, ::grpc::ServerCompletionQueue* cq)
    :service_(service),cq_(cq),responder_(&ctx_),status_(CREATE){
        Proceed();
}
Api_impl::put_rpc_CallData::put_rpc_CallData(key_value_store_rpc::AsyncService* service, ::grpc::ServerCompletionQueue* cq)
    :service_(service),cq_(cq),responder_(&ctx_),status_(CREATE){
        Proceed();
}

void Api_impl::get_rpc_CallData::Proceed() {
    if(status_==CREATE){
        status_=PROCESS;
        service_->Requestget_rpc(&ctx_, &request_, &responder_, cq_, cq_, this);
    } else if(status_==PROCESS){
        new get_rpc_CallData(service_,cq_);
        std::string key=request_.key();
        
        std::pair<std::string,bool> res=(store->GET(key));
        reply_.set_value(res.first);
        reply_.set_ok((bool)res.second);
        status_=FINISH;
        responder_.Finish(reply_,::grpc::Status::OK,this);
    } else{
        delete this;
    }
}
void Api_impl::delete_rpc_CallData::Proceed() {
    if(status_==CREATE){
        status_=PROCESS;
        service_->Requestdelete_rpc(&ctx_, &request_, &responder_, cq_, cq_, this);
    }else if(status_==PROCESS){
        new delete_rpc_CallData(service_,cq_);
        std::string key=request_.key();
        bool res=(store->DELETE(key));
        reply_.set_ok(res);
        status_=FINISH;
        responder_.Finish(reply_,::grpc::Status::OK,this);
    } else{
        delete this;
    }
}
void Api_impl::put_rpc_CallData::Proceed() {
    if(status_==CREATE){
        status_=PROCESS;
        service_->Requestput_rpc(&ctx_, &request_, &responder_, cq_, cq_, this);
    }else if(status_==PROCESS){
        new put_rpc_CallData(service_,cq_);
        std::pair<std::string,std::string> key_value=std::make_pair(request_.key(),request_.value());
        bool res=(store->PUT(key_value));
        reply_.set_ok(res);
        status_=FINISH;
        responder_.Finish(reply_,::grpc::Status::OK,this);

    } else{
        delete this;
    }
}

void Api_impl::HandleRpcs() {
    void* tag;
    bool ok;
    while (cq_->Next(&tag, &ok)) {
        if (ok) {
            static_cast<CallDataBase*>(tag)->Proceed(); 
        } else {
            delete static_cast<CallDataBase*>(tag);
        }
    }
}