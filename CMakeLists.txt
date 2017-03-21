cmake_minimum_required(VERSION 3.6)
project(slkcached)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c hashmap.h hashmap.c buffer.h buffer.c)
add_executable(slkcached ${SOURCE_FILES})