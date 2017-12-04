cmake_minimum_required(VERSION 3.5)
project(MsgQueue)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -g")

include_directories(${PROJECT_SOURCE_DIR}/include)
add_subdirectory(src)
#add_subdirectory(test)

set(PROJ_BUILD_DIR ${PROJECT_SOURCE_DIR}/build)
set(${PROJECT_NAME}_BINARY_DIR ${PROJ_BUILD_DIR})
set(PROJECT_BINARY_DIR ${PROJ_BUILD_DIR})


#add_library(echo_server SHARED ${SRC})
#add_executable(echo_server ${SOURCE_FILES})