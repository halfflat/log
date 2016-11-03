cmake_minimum_required(VERSION 2.8.11)
project(log)
enable_language(CXX)

set(CMAKE_CXX_FLAGS "-std=c++11 -pthread")

add_subdirectory(log)
add_subdirectory(test)
