cmake_minimum_required(VERSION 3.6)
project(memcached)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c hashmap.h hashmap.c buffer.h buffer.c memory.h memory.c)
add_executable(memcached ${SOURCE_FILES})
target_link_libraries(memcached event)
