cmake_minimum_required(VERSION 3.15)
project(hft-orderbook VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for performance
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-O3 -march=native -Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Core C++ library
add_library(orderbook STATIC
    src/orderbook.cpp
)

target_include_directories(orderbook PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# Python bindings with pybind11
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)

if(BUILD_PYTHON_BINDINGS)
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    
    # Try to find pybind11
    find_package(pybind11 CONFIG QUIET)
    
    if(NOT pybind11_FOUND)
        # Download pybind11 if not found
        include(FetchContent)
        FetchContent_Declare(
            pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG v2.11.1
        )
        FetchContent_MakeAvailable(pybind11)
    endif()
    
    pybind11_add_module(pyorderbook python/bindings.cpp)
    target_link_libraries(pyorderbook PRIVATE orderbook)
    
    # Set output directory for Python module
    set_target_properties(pyorderbook PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/python
    )
endif()

# C++ tests
option(BUILD_TESTS "Build C++ tests" ON)

if(BUILD_TESTS)
    enable_testing()
    
    # Download Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    add_executable(orderbook_test
        tests/test_orderbook.cpp
    )
    
    target_link_libraries(orderbook_test
        orderbook
        gtest_main
        gmock_main
    )
    
    include(GoogleTest)
    gtest_discover_tests(orderbook_test)
endif()

# Benchmark executable
option(BUILD_BENCHMARKS "Build benchmarks" ON)

if(BUILD_BENCHMARKS)
    add_executable(orderbook_benchmark
        benchmarks/benchmark.cpp
    )
    
    target_link_libraries(orderbook_benchmark orderbook)
endif()
