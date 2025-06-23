#include <grpcpp/grpcpp.h>

#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "store.h"
#include "api.h"
#include <string>
#include <memory>
#include <utility>
#include "state_machine.h"
#include "client.h"

Store* store;
nuraft::ptr<nuraft::raft_server> raft_instance;
nuraft::ptr<in_mem_state_manager> smgr;

Api_impl::Api_impl(std::string &server_address,nuraft::ptr<nuraft::raft_server> &raft_instance_,Store* store_,nuraft::ptr<state_mgr> &smgr_)
:server_address(server_address){
    store=store_;
    raft_instance=raft_instance_;
    smgr=std::static_pointer_cast<in_mem_state_manager>(smgr_);
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

        if (!raft_instance->is_leader()) {
            int leader_id = raft_instance->get_leader();
            if (leader_id < 0) {
                reply_.set_ok(false);
                status_ = FINISH;
                responder_.Finish(reply_, grpc::Status::OK, this);
                return;
            }
            auto leader_config = smgr->get_srv_config(leader_id);
            std::string leader_address = leader_config->get_endpoint(); 
            Client client(leader_address);
            auto key=std::string(request_.key());
            client.DELETE(key);
        } else {
            key_value_store_state_machine::key_value_payload payload;
            payload.op_type=1;
            payload.key=request_.key();
            
            nuraft::ptr<nuraft::buffer> data = key_value_store_state_machine::enc_log(payload);
            std::vector<nuraft::ptr<nuraft::buffer>> entries = { data };
            nuraft::raft_server::req_ext_params params;
            raft_instance->append_entries_ext(entries,params);
        }

        reply_.set_ok((bool)1);
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
        
        if (!raft_instance->is_leader()) {
            int leader_id = raft_instance->get_leader();
            if (leader_id < 0) {
                reply_.set_ok(false);
                status_ = FINISH;
                responder_.Finish(reply_, grpc::Status::OK, this);
                return;
            }
            auto leader_config = smgr->get_srv_config(leader_id);
            std::string leader_address = leader_config->get_endpoint(); 
            Client client(leader_address);
            std::pair<std::string,std::string> key_value=std::make_pair(request_.key(),request_.value());
            client.PUT(key_value);
        } else {
            key_value_store_state_machine::key_value_payload payload;
            payload.op_type=0;
            payload.key=request_.key();
            payload.value=request_.value();
            nuraft::ptr<nuraft::buffer> data = key_value_store_state_machine::enc_log(payload);

            std::vector<nuraft::ptr<nuraft::buffer>> entries = { data };
            nuraft::raft_server::req_ext_params params;
            raft_instance->append_entries_ext(entries,params);
        }
        reply_.set_ok((bool)1);
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