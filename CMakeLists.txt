cmake_minimum_required(VERSION 3.16)
project(RISC_V_Simulator)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_definitions(-O3)

add_executable(code
        CMakeLists.txt
        src/include.hpp
        src/cpu_core.hpp
        src/cpu_stages.cpp
        src/decoder.hpp
        src/basic_components.hpp
        src/advanced_components.hpp
        src/main.cpp
        )
