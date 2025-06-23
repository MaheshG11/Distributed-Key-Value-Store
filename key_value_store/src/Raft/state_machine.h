#ifndef STATE_MACHINE
#define STATE_MACHINE

#include "nuraft.hxx"

#include <atomic>
#include <cassert>
#include <mutex>

#include <string.h>
#include "store.h"
using namespace nuraft; 

class key_value_store_state_machine: public state_machine {
public:
    key_value_store_state_machine(Store* &store,bool async_snapshot=false);
    ~key_value_store_state_machine();
    struct key_value_payload{
        int op_type;// 0: put, 1: delete, 2: get
        std::string key,value;
    };
    static ptr<buffer> enc_log(const key_value_payload& payload);

    static void dec_log(buffer& log, key_value_payload& payload_out);
    ptr<buffer> pre_commit(const ulong log_idx, buffer& data);
    ptr<buffer> commit(const ulong log_idx, buffer& data) ;
    void commit_config(const ulong log_idx, ptr<cluster_config>& new_conf);
    void rollback(const ulong log_idx, buffer& data);
        int read_logical_snp_obj(snapshot& s,
                             void*& user_snp_ctx,
                             ulong obj_id,
                             ptr<buffer>& data_out,
                             bool& is_last_obj);
    void save_logical_snp_obj(snapshot& s,
                              ulong& obj_id,
                              buffer& data,
                              bool is_first_obj,
                              bool is_last_obj);
    bool apply_snapshot(snapshot& s);
    void free_user_snp_ctx(void*& user_snp_ctx);
    ptr<snapshot> last_snapshot();
    ulong last_commit_index() ;

    void create_snapshot(snapshot& s,
                         async_result<bool>::handler_type& when_done);
    private:
    struct snapshot_ctx {
        snapshot_ctx( ptr<snapshot>& s, int64_t v )
            : snapshot_(s), value_(v) {}
        ptr<snapshot> snapshot_;
        int64_t value_;
    };
    void create_snapshot_internal(ptr<snapshot> ss);
    void create_snapshot_sync(snapshot& s,
                              async_result<bool>::handler_type& when_done);
    void create_snapshot_async(snapshot& s,
                               async_result<bool>::handler_type& when_done);

    std::atomic<int64_t> cur_value_;

    // Last committed Raft log number.
    std::atomic<uint64_t> last_committed_idx_;

    // Keeps the last 3 snapshots, by their Raft log numbers.
    std::map< uint64_t, ptr<snapshot_ctx> > snapshots_;

    // Mutex for `snapshots_`.
    std::mutex snapshots_lock_;

    // If `true`, snapshot will be created asynchronously.
    bool async_snapshot_;

    Store* store;


};



#endif