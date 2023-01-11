cmake_minimum_required(VERSION 3.25.1)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

project(db_proxy)

SET(COMMON_FILES src/utils.cpp)
SET(CLIENT_FILES src/client.cpp src/tcp_client.cpp)
SET(SERVER_FILES src/server.cpp src/tcp_server.cpp src/work_queue.cpp)

add_executable(client ${CLIENT_FILES} ${COMMON_FILES})
add_executable(server ${SERVER_FILES} ${COMMON_FILES})