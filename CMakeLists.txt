cmake_minimum_required(VERSION 3.10)
project(Raw)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall")
set(CMAKE_CXX_STANDARD 14)

add_executable(tests tests.cpp)
