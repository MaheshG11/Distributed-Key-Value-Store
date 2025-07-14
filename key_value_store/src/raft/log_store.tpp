
template <typename T>
logStore<T>::logStore(raftManager &raft_manager,
        std::unique_lock<std::mutex> &raft_manager_lock
            ):raft_manager(raft_manager),
            log_queue_lock(log_queue_mutex,std::defer_lock),
            raft_manager_lock(raft_manager_lock){
                force_quit = false;
                deque_thread_status=false;
                raft_manager_lock.lock();
                if(raft_manager.get_state()==MASTER){
                    start_deque_thread();
                }
                raft_manager_lock.unlock();
            }

template <typename T>
logStore<T>::~logStore(){}

template <typename T>
void logStore<T>::append_entry(T request){
    log_queue_lock.lock();
    log_queue.push(request);
    log_queue_lock.unlock();
    log_queue_cv.notify_one();
}

template <typename T>
void logStore<T>::start_deque_thread(){
    force_quit = true;
    log_queue_cv.wait(log_queue_lock,[this](){
            return deque_thread_status==false;
        }
    )
    deque_thread=std::thread(&logStore::start_,this);
    log_queue_lock.unlock();
}

template <typename T>
void logStore<T>::stop_deque_thread(){
    deque_thread_status = false;
    force_quit=true;
    log_queue_cv.notify_one();
    deque_thread.join();
}
template <typename T>
void logStore<T>::commit(int64_t entry_id_, bool commit){
    log_queue_lock.lock();
    if(!commit){
        if(log_queue.size()){
            log_queue.pop();
        }
        log_queue_lock.unlock();
        return;
    }

    
    if(log_queue.size()){
        T request=log_queue.front();
        log_queue.pop();
        log_queue_lock.unlock();
        if(request.entry_id()!=entry_id_) return;
        execute_entry(request);
    }else{
        log_queue_lock.unlock();
    }
}

template <typename T>
void logStore<T>::start_(){
    deque_thread_status=true;
    force_quit=false;
    while(deque_thread_status){
        log_queue_cv.wait(log_queue_lock,[this](){
            return log_queue.size()>0 || force_quit;
        })
        log_queue_lock.unlock();
        raft_manager_lock.lock();
        if(force_quit || raft_manager.get_state()!=MASTER){
            raft_manager_lock.unlock();
            break;
        } else {
            raft_manager_lock.unlock();
        }
        log_queue_lock.lock();
        T request = log_queue.front();
        log_queue.pop();
        log_queue_lock.unlock();
        request.set_entry_id(entry_id);
        raft_manager_lock.lock();
        if()
        {
            bool is_commited=false;
            if(raft_manager.broadcast_log_entry(request)){
                if(raft_manager.broadcast_commit(entry_id,true)){
                    entry_id++;
                    is_commited=true;
                }
            } else if(is_commited==false){
                raft_manager.broadcast_commit(entry_id,is_commited);
            }
            if(is_commited)execute_entry(request);
        }
        raft_manager_lock.unlock();
        
    }
    deque_thread_status = false;
    log_queue_cv.notify_all();
}
