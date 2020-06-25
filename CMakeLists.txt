cmake_minimum_required(VERSION 3.17)

project(FileTransfer)

# Using C++17 for filesystem API.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_DIR src)
set(SRCS_COMMON
    ${SRC_DIR}/ft_utils.cpp
    ${SRC_DIR}/file/ft_file.cpp
    ${SRC_DIR}/file/ft_file_meta.cpp
    ${SRC_DIR}/protocol/ft_msg.cpp
    ${SRC_DIR}/protocol/ft_msg_fctry.cpp
    ${SRC_DIR}/netwrk/ft_conn.cpp
    ${SRC_DIR}/netwrk/ft_conn_utils.cpp
    ${SRC_DIR}/loop/ft_poll_grp.cpp
    ${SRC_DIR}/loop/ft_signal.cpp
    ${SRC_DIR}/request/ft_req_brkr.cpp
)

set(SRCS_SERVER
    ${SRC_DIR}/ft_server.cpp
    ${SRC_DIR}/netwrk/ft_conn_listener.cpp    
)

set(SRCS_CLIENT
    ${SRC_DIR}/ft_client.cpp
)

link_directories(cryptopp-CRYPTOPP_8_2_0/)
set(LIBS_COMMON cryptopp)

include_directories(cryptopp-CRYPTOPP_8_2_0)
include_directories(${SRC_DIR})

add_executable(ft_server ${SRCS_COMMON} ${SRCS_SERVER})
target_link_libraries(ft_server ${LIBS_COMMON})

add_executable(ft_client ${SRCS_COMMON} ${SRCS_CLIENT})
target_link_libraries(ft_client ${LIBS_COMMON})