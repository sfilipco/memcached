cmake_minimum_required(VERSION 3.6)

add_library(SrcFiles hashmap.h hashmap.c buffer.h buffer.c memory.h memory.c)
add_executable(memcached main.c)
target_link_libraries(memcached SrcFiles event)
