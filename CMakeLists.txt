cmake_minimum_required(VERSION 3.25.1)

project(cmake_template_project)

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS}") # libs
add_executable(build_file src/main.cpp)