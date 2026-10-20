cmake_minimum_required(VERSION 3.20)

project(distributed_kv)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kv_server
    src/main.cpp
    src/kv_store.cpp
    src/parser.cpp
    src/tcp_server.cpp
    src/wal.cpp
    src/snapshot.cpp
)

target_include_directories(kv_server PRIVATE include)
target_link_libraries(kv_server PRIVATE ws2_32)

add_executable(kv_client
    src/tcp_client.cpp
)

target_link_libraries(kv_client PRIVATE ws2_32)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(kv_tests
    tests/kv_store_test.cpp
    tests/parser_test.cpp
    tests/wal_test.cpp
    tests/snapshot_test.cpp
    src/kv_store.cpp
    src/parser.cpp
    src/wal.cpp
    src/snapshot.cpp
)

target_include_directories(kv_tests PRIVATE include)

target_link_libraries(kv_tests
    GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(kv_tests)