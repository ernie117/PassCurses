cmake_minimum_required(VERSION 3.6)

project(PassCurses)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-lncurses")

include_directories(includes)

link_libraries(stdc++fs)

add_executable(PassCurses
    src/main.cpp
    src/includes/json.hpp
)
