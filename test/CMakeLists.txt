cmake_minimum_required(VERSION 3.6)


add_executable(run_tests test_main.c)

target_link_libraries(run_tests memcached_src)
