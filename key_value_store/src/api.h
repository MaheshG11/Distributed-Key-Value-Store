#ifndef API
#define API
#include <grpcpp/grpcpp.h>
#include "protofiles/gRPC_Communication.grpc.pb.h"
#include "protofiles/gRPC_Communication.pb.h"
#include "store.h"
#include <string>
#include <libnuraft/nuraft.hxx>
#include "state_machine.h"
#include "in_mem_state_manager.h"

class CallDataBase {
    public:
        virtual void Proceed() = 0;
        virtual ~CallDataBase() = default;
};
class Api_impl {

private:
    
    class get_rpc_CallData:public CallDataBase{
    private:
        key_value_store_rpc::AsyncService* service_;
        ::grpc::ServerCompletionQueue* cq_;
        ::grpc::ServerContext ctx_;
        ::get_delete request_;
        ::get_response reply_;
        ::grpc::ServerAsyncResponseWriter<::get_response >responder_;
        
        enum{CREATE,PROCESS,FINISH} status_;
        
    public:
        void Proceed() override;
        
        get_rpc_CallData(key_value_store_rpc::AsyncService* service, ::grpc::ServerCompletionQueue* cq);
    };
    class delete_rpc_CallData:public CallDataBase{
    private:
        key_value_store_rpc::AsyncService* service_;
        ::grpc::ServerCompletionQueue* cq_;
        ::grpc::ServerContext ctx_;
        ::get_delete request_;
        ::put_delete_response  reply_;
        ::grpc::ServerAsyncResponseWriter<::put_delete_response >responder_;
        enum{CREATE,PROCESS,FINISH} status_;
    public:
        void Proceed() override;
        delete_rpc_CallData(key_value_store_rpc::AsyncService* service, ::grpc::ServerCompletionQueue* cq);
    };
    class put_rpc_CallData:public CallDataBase{
    private:
        key_value_store_rpc::AsyncService* service_;
        ::grpc::ServerCompletionQueue* cq_;
        ::grpc::ServerContext ctx_;
        ::put request_;
        ::put_delete_response reply_;
        ::grpc::ServerAsyncResponseWriter<::put_delete_response >responder_;
        enum{CREATE,PROCESS,FINISH} status_;
    public:
        void Proceed() override;
        put_rpc_CallData(key_value_store_rpc::AsyncService* service, ::grpc::ServerCompletionQueue* cq);
    };
    void HandleRpcs();
    std::unique_ptr<::grpc::ServerCompletionQueue> cq_;
    std::unique_ptr<::grpc::Server> server_;
    key_value_store_rpc::AsyncService service_;
public:
    void Run();
    Api_impl(std::string &server_address,nuraft::ptr<nuraft::raft_server> &raft_instance_,Store* store_,nuraft::ptr<state_mgr> &smgr_);
    std::string server_address;

};

#endif 