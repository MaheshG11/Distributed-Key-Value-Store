#ifndef IN_MEM_LOG_STORE
#define IN_MEM_LOG_STORE
#pragma once
#include <libnuraft/log_store.hxx>
#include <map>
#include <mutex>

class In_mem_log_store : public nuraft::log_store{
public:
    In_mem_log_store();
    ~In_mem_log_store();

    ulong next_slot() const;
    ulong start_index() const;
    nuraft::ptr<nuraft::log_entry> make_clone(const nuraft::ptr<nuraft::log_entry>& entry) const;
    nuraft::ptr<nuraft::log_entry> last_entry() const;
    ulong append(nuraft::ptr<nuraft::log_entry>& entry);
    void write_at(ulong index, nuraft::ptr<nuraft::log_entry>& entry);
    nuraft::ptr<std::vector<nuraft::ptr<nuraft::log_entry>>> log_entries(ulong start, ulong end);
    nuraft::ptr<nuraft::log_entry> entry_at(ulong index);
    ulong term_at(ulong index);
    nuraft::ptr<nuraft::buffer> pack(ulong index, int32_t cnt);
    void apply_pack(ulong index, nuraft::buffer& pack);
    bool compact(ulong last_log_index);
    bool flush();
private:
    mutable std::mutex mu_,logs_lock_;
    std::map<ulong, nuraft::ptr<nuraft::log_entry>> logs_;
    ulong next_slot_;
    
};
#endif