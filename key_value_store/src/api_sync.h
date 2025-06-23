#ifndef API_SYNC
#define API_SYNC
#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "store.h"
#include <string>
#include <libnuraft/nuraft.hxx>
#include "state_machine.h"
#include "in_mem_state_manager.h"

class Api_impl : public key_value_store_rpc :: Service{

private:
    int port;
    Store* store;
    nuraft::ptr<in_mem_state_manager> smgr;
    nuraft::ptr<nuraft::raft_server> raft_instance;
public:
    Api_impl(std::string &db_path,nuraft::ptr<state_mgr> &smgr_,nuraft::ptr<nuraft::raft_server> &raft_instance_);
    ::grpc::Status get_rpc(::grpc::ServerContext* context, const ::get_delete* request, ::get_response* response);
    ::grpc::Status delete_rpc(::grpc::ServerContext* context, const ::get_delete* request, ::put_delete_response* response);
    ::grpc::Status put_rpc(::grpc::ServerContext* context, const ::put* request, ::put_delete_response* response);
 
};

#endif 