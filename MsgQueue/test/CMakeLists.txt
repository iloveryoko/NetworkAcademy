cmake_minimum_required(VERSION 3.5)
project(MsgQueue)
aux_source_directory(. SRC)
add_executable(test_ahandler ${SRC})