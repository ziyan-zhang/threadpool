cmake_minimum_required(VERSION 3.22)
project(threadpool)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(threadpool
        example.cpp
        multi_LSM_multi_thread.cpp
        one_LSM_multi_thread.cpp
        origin.cpp
        thread_pool.h)
