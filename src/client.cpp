#include "client.h"
#include <ostream>

Client::Client(std::string &ip_port){
    stub_=key_value_store_rpc::NewStub(grpc::CreateChannel(
        ip_port, grpc::InsecureChannelCredentials()
    ));
}

bool Client::PUT(std::pair<std::string,std::string> &key_value){
    ::store_request request;
    request.set_key(key_value.first);
    request.set_value(key_value.second);
    grpc::ClientContext context;
    ::store_response response;
    grpc::Status status = stub_->put_rpc(&context,request,&response);
    if(status.ok())return (bool)response.ok();
    else {
        std::cerr<<status.error_message()<<std::endl;
        return false;
    }

}
bool Client::DELETE(std::string &key){
    ::store_request request;
    request.set_key(key);
    grpc::ClientContext context;
    ::store_response response;
    grpc::Status status = stub_->delete_rpc(&context,request,&response);
    if(status.ok())return (bool)response.ok();
    else {
        std::cerr<<status.error_message()<<std::endl;
        return false;
    }
    
}
std::pair<std::string,bool> Client::GET(std::string &key){
    ::store_request request;
    request.set_key(key);
    grpc::ClientContext context;
    ::store_response response;
    grpc::Status status = stub_->get_rpc(&context,request,&response);
    std::pair<std::string,bool> p;
    if(status.ok()){
        std::pair<std::string,bool> p;
        p.second=response.ok();
        p.first=std::string(response.value());
        return p;
    }
    
    std::cerr<<status.error_message()<<std::endl;
    p.second=false;
    return p;
}

