cmake_minimum_required(VERSION 3.30.3)

project(key_value_store)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}:~/.local:~/.local/bin:~/grpc:${CMAKE_SOURCE_DIR}")
find_package(RocksDB REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

set(NURAFT_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/NuRaft/include")
set(NURAFT_LIB_DIR "${PROJECT_SOURCE_DIR}/NuRaft/build")
message("Source Dir: ${NURAFT_INCLUDE_DIR}")
message("Source Dir: ${NURAFT_LIB_DIR}")
include_directories(${NURAFT_INCLUDE_DIR})
link_directories(${NURAFT_LIB_DIR})

include_directories($ENV{HOME}/.local/include)

add_subdirectory(protofiles)


#store
add_library(store STATIC src/store.cpp)
target_link_libraries(store PRIVATE RocksDB::rocksdb)

#raft essentials
add_subdirectory(src/Raft)

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

#api      server
add_library(API STATIC src/api.cpp)
target_include_directories(API 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
        ${PROJECT_SOURCE_DIR}/src/Raft
        ${PROJECT_SOURCE_DIR}/src/
        ${NURAFT_INCLUDE_DIR}/libnuraft
)
target_link_libraries(API 
    PRIVATE 
        store 
        gRPC_Communication_Lib 
        gRPC::grpc++
        raft_essentials
        ${NURAFT_LIB_DIR}/libnuraft.a
        CLIENT

)

#api_sync      server
add_library(API_SYNC STATIC src/api_sync.cpp)
target_include_directories(API_SYNC 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
        ${PROJECT_SOURCE_DIR}/src/Raft
        ${PROJECT_SOURCE_DIR}/src/
        ${NURAFT_INCLUDE_DIR}/libnuraft
)
target_link_libraries(API_SYNC 
    PRIVATE 
        store 
        gRPC_Communication_Lib 
        gRPC::grpc++
        raft_essentials
        ${NURAFT_LIB_DIR}/libnuraft.a
        CLIENT

)



# add_subdirectory(test)
find_package(Boost REQUIRED COMPONENTS system)
set(LIBSSL OpenSSL::SSL ssl)
set(LIBCRYPTO OpenSSL::Crypto crypto)
set(LIBBOOST_SYSTEM Boost::system)
set(LIBDL dl)
set(LIBZ z)
# set(LIBRARY_NAME "nuraft")
set(LIBRARIES
    ${LIBSSL}
    ${LIBCRYPTO}
    ${LIBBOOST_SYSTEM}
    ${LIBDL}
    ${LIBZ})
add_executable(temp src/main_launcher.cpp)

target_link_libraries(temp 
    PRIVATE 
        API
        store
        raft_essentials
        ${NURAFT_LIB_DIR}/libnuraft.a
        ${LIBRARIES}
        ssl
        crypto
)


target_include_directories(temp 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/Raft
        ${PROJECT_SOURCE_DIR}/src/
        ${NURAFT_INCLUDE_DIR}/libnuraft/
)

add_executable(server_sync src/main.cpp)

target_link_libraries(server_sync 
    PRIVATE 
        API_SYNC
        store
        raft_essentials
        ${NURAFT_LIB_DIR}/libnuraft.a
        ${LIBRARIES}
        ssl
        crypto
)


target_include_directories(server_sync 
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src/Raft
        ${PROJECT_SOURCE_DIR}/src/
        ${NURAFT_INCLUDE_DIR}/libnuraft/
)

