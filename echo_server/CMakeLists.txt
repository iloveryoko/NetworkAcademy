cmake_minimum_required(VERSION 3.5)
project(echo_server)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set(SOURCE_FILES main.c thread_pool.c)
add_executable(echo_server ${SOURCE_FILES})