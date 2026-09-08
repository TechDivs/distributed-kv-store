cmake_minimum_required(VERSION 3.20)

project(distributed_kv)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kv_server
    src/main.cpp
    src/kv_store.cpp
)

target_include_directories(kv_server PRIVATE include)

add_executable(kv_tests
    tests/kv_store_test.cpp
    src/kv_store.cpp
)

target_include_directories(kv_tests PRIVATE include)