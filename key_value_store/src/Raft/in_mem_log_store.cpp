#include "in_mem_log_store.h"
#include <cassert>
#include "nuraft.hxx"

In_mem_log_store::In_mem_log_store():next_slot_(1){}
In_mem_log_store::~In_mem_log_store(){}

ulong In_mem_log_store::next_slot() const {
    std::lock_guard<std::mutex> lock(mu_);
    return next_slot_;
}

ulong In_mem_log_store::start_index() const{
    std::lock_guard<std::mutex> lock(mu_);
    return logs_.begin()->first;
}

nuraft::ptr<nuraft::log_entry> In_mem_log_store::make_clone(const nuraft::ptr<nuraft::log_entry>& entry) const {
    // NOTE:
    //   Timestamp is used only when `replicate_log_timestamp_` option is on.
    //   Otherwise, log store does not need to store or load it.
    nuraft::ptr<nuraft::log_entry> clone = nuraft::cs_new<nuraft::log_entry>
                           ( entry->get_term(),
                             nuraft::buffer::clone( entry->get_buf() ),
                             entry->get_val_type(),
                             entry->get_timestamp(),
                             entry->has_crc32(),
                             entry->get_crc32(),
                             false );
    return clone;
}

nuraft::ptr<nuraft::log_entry> In_mem_log_store::last_entry() const{
    ulong next_idx = next_slot();
    std::lock_guard<std::mutex> l(logs_lock_);
    auto entry = logs_.find( next_idx - 1 );
    if (entry == logs_.end()) {
        entry = logs_.find(0);
        if (entry == logs_.end()) {
            return nullptr;  // Nothing in log
        }
    }
    return make_clone(entry->second);
}

ulong In_mem_log_store::append(nuraft::ptr<nuraft::log_entry> &entry)  {
        std::lock_guard<std::mutex> lock(mu_);
        logs_[next_slot_] = entry;
        return next_slot_++;
}


void In_mem_log_store::write_at(ulong index, nuraft::ptr<nuraft::log_entry> &entry){
        std::lock_guard<std::mutex> lock(mu_);
        logs_[index] = entry;
        if (index >= next_slot_) {
            next_slot_ = index + 1;
        }
}

nuraft::ptr< std::vector< nuraft::ptr<nuraft::log_entry> > >
    In_mem_log_store::log_entries(ulong start, ulong end)
{
    nuraft::ptr< std::vector< nuraft::ptr<nuraft::log_entry> > > ret =
        nuraft::cs_new< std::vector< nuraft::ptr<nuraft::log_entry> > >();

    ret->resize(end - start);
    ulong cc=0;
    for (ulong ii = start ; ii < end ; ++ii) {
        nuraft::ptr<nuraft::log_entry> src = nullptr;
        {   std::lock_guard<std::mutex> l(logs_lock_);
            auto entry = logs_.find(ii);
            if (entry == logs_.end()){
                assert(0);
                entry = logs_.find(0);
            }
            
            src = entry->second;
        }
        (*ret)[cc++] = make_clone(src);
    }
    return ret;
}

nuraft::ptr<nuraft::log_entry> In_mem_log_store::entry_at(ulong index) {
    nuraft::ptr<nuraft::log_entry> src = nullptr;
    {   std::lock_guard<std::mutex> l(logs_lock_);
        auto entry = logs_.find(index);
        if (entry == logs_.end()) {
            entry = logs_.find(0);
        }
        src = entry->second;
    }
    return make_clone(src);
}

ulong In_mem_log_store::term_at(ulong index){
    ulong term = 0;
    {   std::lock_guard<std::mutex> l(logs_lock_);
        auto entry = logs_.find(index);
        if (entry == logs_.end()) {
            entry = logs_.find(0);
        }
        term = entry->second->get_term();
    }
    return term;
}

nuraft::ptr<nuraft::buffer> In_mem_log_store::pack(ulong index, int32_t cnt) {
    std::vector< nuraft::ptr<nuraft::buffer> > logs;

    size_t size_total = 0;
    for (ulong ii=index; ii<index+cnt; ++ii) {
        nuraft::ptr<nuraft::log_entry> le = nullptr;
        {   std::lock_guard<std::mutex> l(logs_lock_);
            le = logs_[ii];
        }
        assert(le.get());
        nuraft::ptr<nuraft::buffer> buf = le->serialize();
        size_total += buf->size();
        logs.push_back( buf );
    }

    nuraft::ptr<nuraft::buffer> buf_out = nuraft::buffer::alloc
                          ( sizeof(int32_t) +
                            cnt * sizeof(int32_t) +
                            size_total );
    buf_out->pos(0);
    buf_out->put((int32_t)cnt);

    for (auto& entry: logs) {
        nuraft::ptr<nuraft::buffer>& bb = entry;
        buf_out->put((int32_t)bb->size());
        buf_out->put(*bb);
    }
    return buf_out;
}

void In_mem_log_store::apply_pack(ulong index, nuraft::buffer& pack) {
    pack.pos(0);
    int32_t num_logs = pack.get_int();

    for (int32_t ii=0; ii<num_logs; ++ii) {
        ulong cur_idx = index + ii;
        int32_t buf_size = pack.get_int();

        nuraft::ptr<nuraft::buffer> buf_local = nuraft::buffer::alloc(buf_size);
        pack.get(buf_local);

        nuraft::ptr<nuraft::log_entry> le = nuraft::log_entry::deserialize(*buf_local);
        {   std::lock_guard<std::mutex> l(logs_lock_);
            logs_[cur_idx] = le;
        }
    }

    {   std::lock_guard<std::mutex> l(logs_lock_);
        auto entry = logs_.end();
        if(logs_.size()){
            entry--;
            next_slot_ = entry->first;
        } else {
            next_slot_ = 1;
        }
    }
}

bool In_mem_log_store::compact(ulong last_log_index) {
    std::lock_guard<std::mutex> l(logs_lock_);
    for(auto entry=logs_.begin(); (entry->first)<=last_log_index || entry!=logs_.end();){
        auto entry_=entry;
        entry++;
        logs_.erase(entry_);
    }

    // WARNING:
    //   Even though nothing has been erased,
    //   we should set `start_idx_` to new index.
    if (next_slot_ <= last_log_index) {
        next_slot_ = last_log_index + 1;
    }
    return true;
}

bool In_mem_log_store::flush(){return true;}