add_executable(start_test 
    # store_test.cpp
    raft_queue_test.cpp
)
target_link_libraries(start_test 
    PRIVATE 
        # Add the libraries which are being tested
        RAFT_QUEUE
        # Add the libraries on which the tests depends
        gtest 
        gtest_main 
        pthread
        gRPC_Communication_Lib 
        gRPC::grpc++
        spdlog::spdlog_header_only
        RocksDB::rocksdb
        
)

target_include_directories(start_test 
    PUBLIC 
        # add searchable '.h' file's paths
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/raft
        ${PROJECT_SOURCE_DIR}/src/raft/queue
)
add_compile_definitions(
    SPDLOG_ENABLE_SOURCE_LOC
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
)
