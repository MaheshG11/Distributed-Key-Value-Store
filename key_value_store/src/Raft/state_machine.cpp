#include "state_machine.h"
#include "store.h"

#include "utility"

key_value_store_state_machine::key_value_store_state_machine(Store* &store,bool async_snapshot)
    : cur_value_(0)
    , last_committed_idx_(0)
    , async_snapshot_(async_snapshot)
    , store(store){}

key_value_store_state_machine::~key_value_store_state_machine(){
        delete store; 
}

ptr<buffer> key_value_store_state_machine::enc_log(const key_value_payload& payload) {
        // Encode from {operator, operand} to Raft log.
    size_t total_size = sizeof(int) + 4 + payload.key.size() + 4 + payload.value.size();
    ptr<buffer> ret = buffer::alloc(total_size);
    buffer_serializer bs(ret);

    bs.put_i32(payload.op_type);
    bs.put_str(payload.key);
    bs.put_str(payload.value);

    return ret;
    }

void key_value_store_state_machine::dec_log(buffer& log, key_value_payload& payload_out) {
    buffer_serializer bs(log);

    payload_out.op_type = bs.get_i32();
    payload_out.key = bs.get_str();
    payload_out.value = bs.get_str();
    }

ptr<buffer> key_value_store_state_machine::pre_commit(const ulong log_idx, buffer& data) {
        // Nothing to do with pre-commit in this example.
        return nullptr;
}

ptr<buffer> key_value_store_state_machine::commit(const ulong log_idx, buffer& data) {
        key_value_payload payload;
        dec_log(data, payload);
        
        bool status=0;
        std::pair<std::string,std::string> p;
        switch (payload.op_type) {
        case 0:   
            p={payload.key,payload.value};
            status=store->PUT(p);
            break;
        case 1:
            status=store->DELETE(payload.key);
            break;
        default:
            return nullptr;
        }
        last_committed_idx_ = log_idx;

        // Return Raft log number as a return result.
        ptr<buffer> ret = buffer::alloc( sizeof(log_idx) );
        buffer_serializer bs(ret);
        bs.put_u64(log_idx);
        return ret;
}

void key_value_store_state_machine::commit_config(const ulong log_idx, ptr<cluster_config>& new_conf) {
    // Nothing to do with configuration change. Just update committed index.
    last_committed_idx_ = log_idx;
}



void key_value_store_state_machine::rollback(const ulong log_idx, buffer& data) {
    // Nothing to do with rollback,
    // as this example doesn't do anything on pre-commit.
}

int key_value_store_state_machine::read_logical_snp_obj(snapshot& s,
                         void*& user_snp_ctx,
                         ulong obj_id,
                         ptr<buffer>& data_out,
                         bool& is_last_obj)
{
    ptr<snapshot_ctx> ctx = nullptr;
    {   std::lock_guard<std::mutex> ll(snapshots_lock_);
        auto entry = snapshots_.find(s.get_last_log_idx());
        if (entry == snapshots_.end()) {
            // Snapshot doesn't exist.
            //
            // NOTE:
            //   This should not happen in real use cases.
            //   The below code is an example about how to handle
            //   this case without aborting the server.
            data_out = nullptr;
            is_last_obj = true;
            return -1;
        }
        ctx = entry->second;
    }
    if (obj_id == 0) {
        // Object ID == 0: first object, put dummy data.
        data_out = buffer::alloc( sizeof(int32) );
        buffer_serializer bs(data_out);
        bs.put_i32(0);
        is_last_obj = false;
    } else {
        // Object ID > 0: second object, put actual value.
        data_out = buffer::alloc( sizeof(ulong) );
        buffer_serializer bs(data_out);
        bs.put_u64( ctx->value_ );
        is_last_obj = true;
    }
    return 0;
}

void key_value_store_state_machine::save_logical_snp_obj(snapshot& s,
                          ulong& obj_id,
                          buffer& data,
                          bool is_first_obj,
                          bool is_last_obj)
{
    if (obj_id == 0) {
        // Object ID == 0: it contains dummy value, create snapshot context.
        ptr<buffer> snp_buf = s.serialize();
        ptr<snapshot> ss = snapshot::deserialize(*snp_buf);
        create_snapshot_internal(ss);
    } else {
        // Object ID > 0: actual snapshot value.
        buffer_serializer bs(data);
        int64_t local_value = (int64_t)bs.get_u64();
        std::lock_guard<std::mutex> ll(snapshots_lock_);
        auto entry = snapshots_.find(s.get_last_log_idx());
        assert(entry != snapshots_.end());
        entry->second->value_ = local_value;
    }
    // Request next object.
    obj_id++;
}

bool key_value_store_state_machine::apply_snapshot(snapshot& s) {
    std::lock_guard<std::mutex> ll(snapshots_lock_);
    auto entry = snapshots_.find(s.get_last_log_idx());
    if (entry == snapshots_.end()) return false;
    ptr<snapshot_ctx> ctx = entry->second;
    cur_value_ = ctx->value_;
    return true;
}

void key_value_store_state_machine::free_user_snp_ctx(void*& user_snp_ctx) {
    // In this example, `read_logical_snp_obj` doesn't create
    // `user_snp_ctx`. Nothing to do in this function.
}

ptr<snapshot> key_value_store_state_machine::last_snapshot() {
    // Just return the latest snapshot.
    std::lock_guard<std::mutex> ll(snapshots_lock_);
    auto entry = snapshots_.rbegin();
    if (entry == snapshots_.rend()) return nullptr;

    ptr<snapshot_ctx> ctx = entry->second;
    return ctx->snapshot_;
}

ulong key_value_store_state_machine::last_commit_index() {
    return last_committed_idx_;
}

void key_value_store_state_machine::create_snapshot(snapshot& s,
                     async_result<bool>::handler_type& when_done)
{
    if (!async_snapshot_) {
        // Create a snapshot in a synchronous way (blocking the thread).
        create_snapshot_sync(s, when_done);
    } else {
        // Create a snapshot in an asynchronous way (in a different thread).
        create_snapshot_async(s, when_done);
    }
}




void key_value_store_state_machine::create_snapshot_internal(ptr<snapshot> ss) {
    std::lock_guard<std::mutex> ll(snapshots_lock_);
    // Put into snapshot map.
    ptr<snapshot_ctx> ctx = cs_new<snapshot_ctx>(ss, cur_value_);
    snapshots_[ss->get_last_log_idx()] = ctx;
    // Maintain last 3 snapshots only.
    const int MAX_SNAPSHOTS = 3;
    int num = snapshots_.size();
    auto entry = snapshots_.begin();
    for (int ii = 0; ii < num - MAX_SNAPSHOTS; ++ii) {
        if (entry == snapshots_.end()) break;
        entry = snapshots_.erase(entry);
    }
}

void key_value_store_state_machine::create_snapshot_sync(snapshot& s,
                          async_result<bool>::handler_type& when_done)
{
    // Clone snapshot from `s`.
    ptr<buffer> snp_buf = s.serialize();
    ptr<snapshot> ss = snapshot::deserialize(*snp_buf);
    create_snapshot_internal(ss);
    ptr<std::exception> except(nullptr);
    bool ret = true;
    when_done(ret, except);

}

void key_value_store_state_machine::create_snapshot_async(snapshot& s,
                           async_result<bool>::handler_type& when_done)
{
    // Clone snapshot from `s`.
    ptr<buffer> snp_buf = s.serialize();
    ptr<snapshot> ss = snapshot::deserialize(*snp_buf);

    // Note that this is a very naive and inefficient example
    // that creates a new thread for each snapshot creation.
    std::thread t_hdl([this, ss, when_done]{
        create_snapshot_internal(ss);

        ptr<std::exception> except(nullptr);
        bool ret = true;
        when_done(ret, except);

    });
    t_hdl.detach();
}


