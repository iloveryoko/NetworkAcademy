cmake_minimum_required(VERSION 3.5)
project(libuv_example)

set(SOURCE_FILES main.c client.c getopt.c s5.c server.c util.c)
add_executable(sock5-proxy ${SOURCE_FILES})
target_link_libraries(sock5-proxy uv)