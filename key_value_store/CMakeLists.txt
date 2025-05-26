cmake_minimum_required(VERSION 3.30.3)



project(key_value_store)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}:~/.local:~/.local/bin:~/grpc:${CMAKE_SOURCE_DIR}")
find_package(RocksDB REQUIRED)
# add_subdirectory(src)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
add_library(store STATIC src/store.cpp)
target_link_libraries(store PRIVATE RocksDB::rocksdb)
add_executable(temp src/main.cpp)
target_link_libraries(temp PRIVATE store)
# target_link_libraries(Node
    # Master
# )
