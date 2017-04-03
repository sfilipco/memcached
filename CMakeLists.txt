cmake_minimum_required(VERSION 3.6)
project(MEMCACHED C)
set(CMAKE_C_STANDARD 11)

add_subdirectory(src)

add_subdirectory(test)
