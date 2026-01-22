#include <grpcpp/grpcpp.h>

#include <string>
#include "api.h"
#include "gRPC_Communication.grpc.pb.h"
#include "gRPC_Communication.pb.h"

Api_impl::Api_impl(
    /*Store& store, std::shared_ptr<logStoreImpl> log_store_ptr*/)
/*store(store), log_store_ptr(log_store_ptr)*/ {}

grpc::Status Api_impl::GetRPC(grpc::ServerContext* context,
                              const StoreRequest* request,
                              StoreResponse* response) {
  std::string key = request->key();
  std::pair<std::string, bool> res = {"a", true}; /*(store.GET(key));*/
  response->set_value(res.first);
  response->set_ok((bool)res.second);
  if (res.second) {
    return grpc::Status::OK;
  }
  return grpc::Status(grpc::StatusCode::NOT_FOUND, "key not found");
}

grpc::Status Api_impl::WriteRPC(grpc::ServerContext* context,
                                const StoreRequest* request,
                                StoreResponse* response) {
  return grpc::Status::OK;
}
