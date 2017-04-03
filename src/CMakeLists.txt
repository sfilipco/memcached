cmake_minimum_required(VERSION 3.6)

add_library(memcached_src hashmap.h hashmap.c memory.h memory.c string_hashmap.h string_hashmap.c util.h)
add_executable(memcached main.c)
target_link_libraries(memcached memcached_src event)
