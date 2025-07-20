cmake_minimum_required(VERSION 3.30.3)

project(key_value_store)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}:~/.local:~/.local/bin:~/grpc:${CMAKE_SOURCE_DIR}")
find_package(RocksDB REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

include_directories($ENV{HOME}/.local/include)

add_subdirectory(protofiles)

add_subdirectory(src/raft)

#store
add_library(store STATIC src/store.cpp)
target_link_libraries(store PRIVATE RocksDB::rocksdb)

#api      server
add_library(API STATIC src/api.cpp)
target_include_directories(API 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/raft
)

target_link_libraries(API 
    PUBLIC
        store 
        gRPC_Communication_Lib 
        gRPC::grpc++
        RAFT
)

#LOG STORE IMPL
add_library(LOG_STORE_IMPL STATIC 
        src/log_store_impl.cpp
        src/raft/raft_manager.cpp)
target_include_directories(LOG_STORE_IMPL
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/raft
)

target_link_libraries(LOG_STORE_IMPL
    PUBLIC
        store
        gRPC_Communication_Lib 
        gRPC::grpc++
        RAFT
)

#client
add_library(CLIENT STATIC src/client.cpp)
target_include_directories(CLIENT 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
)
target_link_libraries(CLIENT
    PRIVATE  
        gRPC_Communication_Lib 
        gRPC::grpc++
)

# add_subdirectory(test)

add_executable(distributed_kv_store src/main.cpp 
                )
                
target_include_directories(distributed_kv_store
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/raft
)

target_link_libraries(distributed_kv_store
    PRIVATE 
        API
        LOG_STORE_IMPL
        RAFT
        gRPC_Communication_Lib 
        gRPC::grpc++
)


# target_link_libraries(Node
    # Master
# )
