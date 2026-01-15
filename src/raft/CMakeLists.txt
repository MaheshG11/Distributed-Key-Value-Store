add_library(RAFT STATIC
    # ${CMAKE_CURRENT_SOURCE_DIR}/log_store.h
    ${CMAKE_CURRENT_SOURCE_DIR}/raft_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/raft_manager.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/raft_server.h
)

target_link_libraries(RAFT PUBLIC 
        gRPC_Communication_Lib 
        gRPC::grpc++
)
target_include_directories(RAFT PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/protofiles

)
