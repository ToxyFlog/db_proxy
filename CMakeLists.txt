cmake_minimum_required(VERSION 3.25.1)

project(status_bar)

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -lasound -lX11")
add_executable(${PROJECT_NAME} 
    src/modules/module.cpp src/modules/module.hpp
    src/modules/volume.cpp src/modules/date.cpp src/modules/microphone.cpp src/modules/language.cpp src/modules/modules.hpp 
    src/status_bar.cpp src/status_bar.hpp 
    src/main.cpp)