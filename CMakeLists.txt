SET(COMMON_FILES src/utils.cpp src/write.cpp)
SET(CLIENT_FILES src/clientApp.cpp src/client.cpp)
SET(SERVER_FILES src/main.cpp src/server.cpp src/workers.cpp src/workQueue.cpp src/pgClient.cpp)


cmake_minimum_required(VERSION 3.25.1)
set(CMAKE_CXX_STANDARD 20)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

project(db_proxy)

find_package(PostgreSQL REQUIRED)

add_executable(client ${CLIENT_FILES} ${COMMON_FILES})
add_executable(server ${SERVER_FILES} ${COMMON_FILES})

target_link_libraries(server PRIVATE PostgreSQL::PostgreSQL)