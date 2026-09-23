cmake_minimum_required(VERSION 3.20)
project(miniRedis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MINIREDIS_BUILD_TESTS "Build the GoogleTest unit tests" ON)
option(MINIREDIS_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

if(MINIREDIS_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

find_package(Threads REQUIRED)

# everything under internal/, shared by the server and the tests
add_library(miniredis_internal STATIC
    internal/bufio/reader.cpp
    internal/resp/parser.cpp
    internal/server/tcp.cpp
    internal/storage/engine.cpp
)
target_include_directories(miniredis_internal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(miniredis_internal PUBLIC -Wall -Wextra -Wpedantic)
target_link_libraries(miniredis_internal PUBLIC Threads::Threads)

add_executable(miniredis cmd/server/main.cpp)
target_link_libraries(miniredis PRIVATE miniredis_internal)

if(MINIREDIS_BUILD_TESTS)
    enable_testing()

    # use a system GoogleTest if installed, otherwise download it
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_executable(miniredis_tests
        tests/bufio_test.cpp
        tests/parser_test.cpp
        tests/engine_test.cpp
    )
    target_link_libraries(miniredis_tests PRIVATE miniredis_internal GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(miniredis_tests)
endif()
