#include "in_mem_state_manager.h"

in_mem_state_manager::in_mem_state_manager(int srv_id,
                    const std::string& endpoint)
        : my_id_(srv_id)
        , my_endpoint_(endpoint)
        , cur_log_store_( cs_new<In_mem_log_store>() )
    {
        my_srv_config_ = cs_new<srv_config>( srv_id, endpoint );

        // Initial cluster config: contains only one server (myself).
        saved_config_ = cs_new<cluster_config>();
        saved_config_->get_servers().push_back(my_srv_config_);
    }
in_mem_state_manager::~in_mem_state_manager(){}

ptr<cluster_config> in_mem_state_manager::load_config() {
        // Just return in-memory data in this example.
        // May require reading from disk here, if it has been written to disk.
        return saved_config_;
    }
void in_mem_state_manager::save_config(const cluster_config& config) {
        // Just keep in memory in this example.
        // Need to write to disk here, if want to make it durable.
        ptr<buffer> buf = config.serialize();
        saved_config_ = cluster_config::deserialize(*buf);
        all_srv_configs_.clear();
        for (const auto& srv : config.get_servers()) {
            all_srv_configs_[srv->get_id()] = srv;
        }

}

void in_mem_state_manager::save_state(const srv_state& state) {
        // Just keep in memory in this example.
        // Need to write to disk here, if want to make it durable.
        ptr<buffer> buf = state.serialize();
        saved_state_ = srv_state::deserialize(*buf);
}

ptr<srv_state> in_mem_state_manager::read_state() {
        // Just return in-memory data in this example.
        // May require reading from disk here, if it has been written to disk.
        return saved_state_;
}

ptr<log_store> in_mem_state_manager::load_log_store() {
        return cur_log_store_;
}

int32 in_mem_state_manager::server_id() {
        return my_id_;
}

void in_mem_state_manager::system_exit(const int exit_code) {
}

ptr<srv_config> in_mem_state_manager::get_srv_config(int id) const {
    auto it = all_srv_configs_.find(id);
    if (it != all_srv_configs_.end()) {
        return it->second;
    }
    return nullptr; // or throw/log an error
}