#include <libnuraft/nuraft.hxx>
#include "in_mem_log_store.h"
#include "in_mem_state_manager.h"
#include "state_machine.h"
#include <string>
#include "store.h"
#include "api.h"
#include "iostream"
#include <chrono>
#include "client.h"
using namespace nuraft;

/*
Arguments
1.)async_snapshot : integer 0=false,other=true,
2.) path
3.) ip
4.) port
5.) srv_id
6.) total replicas
*/
int main(int argc, char* argv[]) {
    // bool async_snapshot=std::stoi(std::string(argv[1]));
    // std::string ip(argv[3]);
    // int port=std::stoi(std::string(argv[4]));
    // int srv_id=std::stoi(std::string(argv[5])),n_replicas=std::stoi(std::string(argv[6]));
    // std::string path(argv[2]),endpoint=ip+":"+std::to_string(port+srv_id);
    bool async_snapshot=1;
    std::string ip("0.0.0.0");
    int port=std::stoi(std::string("7000"));
    int srv_id=std::stoi(std::string("1")),n_replicas=std::stoi(std::string("1"));
    std::string path("/data_"),endpoint=ip+":"+std::to_string(port+srv_id);
    Store* store = new Store(path);
    ptr<logger>         my_logger = nullptr;
    ptr<state_machine> sm = cs_new<key_value_store_state_machine>(store,async_snapshot);
    ptr<state_mgr> smgr = cs_new<in_mem_state_manager>(srv_id,endpoint);

    raft_params params;
    params.heart_beat_interval_ = 100;
    params.election_timeout_lower_bound_ = 200;
    params.election_timeout_upper_bound_ = 400;

   
    asio_service::options   asio_opt;   // Use 1 thread for now
    std::cout<<"async_snapshot : "<<async_snapshot<<"\npath: "<<path<<"\nip: "<<ip<<"\nport: "<<port<<"\nsrv_id: "<<srv_id<<"\ntotal replicas: "<<n_replicas<<"\n";
    raft_launcher launcher;
    if (srv_id == 1) {
    ptr<cluster_config> conf = cs_new<cluster_config>(1); // term = 1
    for (int i = 1; i <= n_replicas; ++i) {
        std::string ep = ip + ":" + std::to_string(port + i);
        ptr<srv_config> s = cs_new<srv_config>(i, ep);
        conf->get_servers().push_back(s);
    }
    smgr->save_config(*conf);
}
    ptr<raft_server> raft_instance = launcher.init(
        sm,
        smgr,
        my_logger,
        port,
        asio_opt, // default rpc handler
        params
    );
    
    // // // Start the server listener
    // // launcher.run();
    // Need to wait for initialization.
    while (!raft_instance->is_initialized()) {
        std::this_thread::sleep_for( std::chrono::milliseconds(100) );
    }
    std::cout<<"server initialized"<<std::endl;


    endpoint=ip+":"+std::to_string(port+srv_id-1000);
    // Api_impl service(endpoint,raft_instance,store,smgr);
//     service.Run();
    // after this I will run server where the raft instance is used to append entries
    // like raft_instance->append_entries(some_entry) or i will return directly if the request is get 
    // keep running0
    std::this_thread::sleep_for( std::chrono::milliseconds(10000) );
    return 0;
}