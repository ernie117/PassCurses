cmake_minimum_required(VERSION 3.6)

project(PassCurses)

set(CMAKE_CXX_STANDARD 17)

include_directories(includes)

link_libraries(stdc++fs ncurses)

add_executable(PassCurses
    src/main.cpp
    src/includes/json.hpp
)
