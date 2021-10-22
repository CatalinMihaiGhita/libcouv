cmake_minimum_required(VERSION 3.0.2)
project(libcouv)
add_definitions(-std=c++20 -O3 -Werror -fcoroutines -fext-numeric-literals)
include_directories(include)

add_subdirectory(tests)
