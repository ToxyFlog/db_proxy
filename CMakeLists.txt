cmake_minimum_required(VERSION 3.25.1)

project(db_proxy)

SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

SET(COMMON_FILES src/common.cpp src/common.hpp)
add_executable(client src/client.cpp ${COMMON_FILES})
add_executable(server src/server.cpp ${COMMON_FILES})