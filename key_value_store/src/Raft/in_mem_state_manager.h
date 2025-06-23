#ifndef IN_MEM_STATE_MANAGER
#define IN_MEM_STATE_MANAGER

#include <unordered_map>
#include "in_mem_log_store.h"
#include "nuraft.hxx"
using namespace nuraft;

class in_mem_state_manager :public state_mgr{
public:
    in_mem_state_manager(int srv_id,
                    const std::string& endpoint);
    ~in_mem_state_manager();
    ptr<cluster_config> load_config() ;
    void save_config(const cluster_config& config);
    void save_state(const srv_state& state) ;
    ptr<srv_state> read_state();
    ptr<log_store> load_log_store();
    int32 server_id();
    void system_exit(const int exit_code);
    ptr<srv_config> get_srv_config(int id) const;
private:
    std::unordered_map<int, ptr<srv_config>> all_srv_configs_;
    int my_id_;
    std::string my_endpoint_;
    ptr<In_mem_log_store> cur_log_store_;
    ptr<srv_config> my_srv_config_;
    ptr<cluster_config> saved_config_;
    ptr<srv_state> saved_state_;
};
#endif