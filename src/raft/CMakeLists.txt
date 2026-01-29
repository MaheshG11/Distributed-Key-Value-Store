set(CMAKE_BUILD_TYPE Debug)


include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
        ${PROJECT_SOURCE_DIR}/src/raft
        ${CMAKE_CURRENT_SOURCE_DIR}/cluster
        ${CMAKE_CURRENT_SOURCE_DIR}/election
        ${CMAKE_CURRENT_SOURCE_DIR}/heartbeat
        ${CMAKE_CURRENT_SOURCE_DIR}/network
        ${CMAKE_CURRENT_SOURCE_DIR}/kv_api
        ${CMAKE_CURRENT_SOURCE_DIR}/state
        ${CMAKE_CURRENT_SOURCE_DIR}/queue

)


# API
add_library(API OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/kv_api/api.cpp
)

target_include_directories(API 
    PUBLIC 
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/protofiles
            ${CMAKE_CURRENT_SOURCE_DIR}/kv_api

${CMAKE_CURRENT_SOURCE_DIR}/queue/

        ${PROJECT_SOURCE_DIR}/src/raft
)

# target_link_libraries(API 
#     PUBLIC
#         # store 
#         gRPC_Communication_Lib 
#         gRPC::grpc++
# )
target_compile_definitions(API
    PUBLIC 
    SPDLOG_ENABLE_SOURCE_LOC
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
)


add_library(RAFT_QUEUE OBJECT 
${CMAKE_CURRENT_SOURCE_DIR}/queue/log_queue.cpp
${CMAKE_CURRENT_SOURCE_DIR}/queue/store.cpp

)


# target_link_libraries(RAFT_QUEUE 
#     PUBLIC
#         # store 
#         gRPC_Communication_Lib 
#         gRPC::grpc++
        
# )
target_compile_definitions(RAFT_QUEUE
    PRIVATE 
    SPDLOG_ENABLE_SOURCE_LOC
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG

)




# RAFT_SUPPORT
add_library(RAFT_SUPPORT OBJECT
${CMAKE_CURRENT_SOURCE_DIR}/network/rpc_calls.cpp
)






# target_link_libraries(RAFT_SUPPORT 
#     PUBLIC
#         # store 
#         gRPC_Communication_Lib 
#         gRPC::grpc++
# )
target_compile_definitions(RAFT_SUPPORT
    PRIVATE 
    SPDLOG_ENABLE_SOURCE_LOC
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG

)

# RAFT_FEATURES
add_library(RAFT_FEATURES OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/cluster/cluster_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/election/election.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/heartbeat/heartbeat_sensor.cpp
)

# target_link_libraries(RAFT_FEATURES 
#     PUBLIC
#         # store 
#         gRPC_Communication_Lib 
#         gRPC::grpc++
# )
target_compile_definitions(RAFT_FEATURES
    PRIVATE 
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
    SPDLOG_ENABLE_SOURCE_LOC
)


#RAFT
add_library(RAFT STATIC
    # ${CMAKE_CURRENT_SOURCE_DIR}/log_store.h
    ${CMAKE_CURRENT_SOURCE_DIR}/raft_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raft_dtypes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raft_server.cpp
    # ${CMAKE_CURRENT_SOURCE_DIR}/raft_server.h
)

target_link_libraries(RAFT PUBLIC 
        gRPC_Communication_Lib 
        gRPC::grpc++
        spdlog::spdlog
        API
        RAFT_FEATURES
        RAFT_SUPPORT
        RAFT_QUEUE
        RocksDB::rocksdb

)

target_compile_definitions(RAFT
    PRIVATE 
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG
    SPDLOG_ENABLE_SOURCE_LOC
)

