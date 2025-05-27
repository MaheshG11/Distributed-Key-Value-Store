cmake_minimum_required(VERSION 3.30.3)

project(key_value_store)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}:~/.local:~/.local/bin:~/grpc:${CMAKE_SOURCE_DIR}")
find_package(RocksDB REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_subdirectory(protofiles)
add_library(API STATIC src/api.cpp)
add_library(store STATIC src/store.cpp)
target_link_libraries(store PRIVATE RocksDB::rocksdb)
target_include_directories(API 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
)
include_directories($ENV{HOME}/.local/include)
target_link_libraries(API 
    PRIVATE 
        store 
        gRPC_Communication_Lib 
        gRPC::grpc++
)


add_executable(temp src/main.cpp)

target_link_libraries(temp PRIVATE API)
# target_link_libraries(Node
    # Master
# )
