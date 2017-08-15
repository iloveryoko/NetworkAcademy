cmake_minimum_required(VERSION 3.5)
project(libuv_example)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(BUILD_DIR ${PROJECT_SOURCE_DIR}/build)
set(CMAKE_BINARY_DIR ${BUILD_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)
link_directories(${PROJECT_SOURCE_DIR}/libs)
#install(DIRECTORY ${CMAKE_SOURCE_DIR}/libs/ DESTINATION ${BUILD_DIR})
file(COPY "${PROJECT_SOURCE_DIR}/libs/" DESTINATION "${BUILD_DIR}")

add_subdirectory(src)