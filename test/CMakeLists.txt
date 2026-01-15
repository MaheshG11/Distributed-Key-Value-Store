add_executable(start_test 
    store_test.cpp
    api_test.cpp
    raft_manager_test.cpp
)
target_link_libraries(start_test 
    PRIVATE 
        # Add the libraries which are being tested
        store
        API
        RAFT
        # Add the libraries on which the tests depends
        gtest 
        gtest_main 
        pthread
        gRPC_Communication_Lib 
        gRPC::grpc++
        
)

target_include_directories(start_test 
    PUBLIC 
        # add searchable '.h' file's paths
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/raft
)