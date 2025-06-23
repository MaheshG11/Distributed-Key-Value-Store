add_library(raft_essentials STATIC in_mem_log_store.cpp in_mem_state_manager.cpp state_machine.cpp)
target_link_libraries(raft_essentials 
    PUBLIC 
        store
        ${NURAFT_LIB_DIR}/libnuraft.a
        )
target_include_directories(raft_essentials 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}/src
        ${NURAFT_INCLUDE_DIR}/libnuraft/
)