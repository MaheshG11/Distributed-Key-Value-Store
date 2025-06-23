#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "store.h"
#include "api_sync.h"
#include <string>
#include <utility>
#include "client.h"
Api_impl::Api_impl(std::string &db_path,nuraft::ptr<state_mgr> &smgr_,nuraft::ptr<nuraft::raft_server> &raft_instance_):raft_instance(raft_instance_){
    store=new Store(db_path);
    smgr=std::static_pointer_cast<in_mem_state_manager>(smgr_);
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
    bool res;
    if (!raft_instance->is_leader()) {
        int leader_id = raft_instance->get_leader();
        if (leader_id < 0) {
            response->set_ok(false);
            return grpc::Status::OK;
        }
        auto leader_config = smgr->get_srv_config(leader_id);
        std::string leader_address = leader_config->get_endpoint(); 
        Client client(leader_address);
        client.DELETE(key);
    } else {
        key_value_store_state_machine::key_value_payload payload;
        payload.op_type=1;
        payload.key=key;
        
        nuraft::ptr<nuraft::buffer> data = key_value_store_state_machine::enc_log(payload);
        std::vector<nuraft::ptr<nuraft::buffer>> entries = { data };
        nuraft::raft_server::req_ext_params params;
        raft_instance->append_entries_ext(entries,params);
    }
    if(res){
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "key not found");
}

::grpc::Status Api_impl::put_rpc(::grpc::ServerContext* context, const ::put* request, ::put_delete_response* response){
    std::pair<std::string,std::string> key_value=std::make_pair(request->key(),request->value());
    bool res=false;
    if (!raft_instance->is_leader()) {
        int leader_id = raft_instance->get_leader();
        if (leader_id < 0) {
            response->set_ok(res);
            return grpc::Status::OK;
        }
        auto leader_config = smgr->get_srv_config(leader_id);
        std::string leader_address = leader_config->get_endpoint(); 
        Client client(leader_address);
        std::pair<std::string,std::string> key_value=std::make_pair(request->key(),request->value());
        client.PUT(key_value);
    } else {
        key_value_store_state_machine::key_value_payload payload;
        payload.op_type=0;
        payload.key=request->key();
        payload.value=request->value();
        nuraft::ptr<nuraft::buffer> data = key_value_store_state_machine::enc_log(payload);
        std::vector<nuraft::ptr<nuraft::buffer>> entries = { data };
        nuraft::raft_server::req_ext_params params;
        raft_instance->append_entries_ext(entries,params);
        res=1;
    }
    response->set_ok(res);
    if(res){
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "INTERNAL_SERVER_ERROR");
}