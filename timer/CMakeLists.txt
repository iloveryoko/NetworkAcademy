cmake_minimum_required(VERSION 3.5)
project(timer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set(SOURCE_FILES main.cpp Timer.cpp TimerManager.cpp)
add_executable(timer ${SOURCE_FILES})