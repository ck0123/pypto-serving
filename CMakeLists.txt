cmake_minimum_required(VERSION 3.20)
project(pypto-serving CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_TESTS "Build unit and integration tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

# Dependencies
find_package(Threads REQUIRED)

# Source files
set(PYPTO_SOURCES
    src/common/config.cpp
    src/common/logger.cpp
    src/engine/sequence.cpp
    src/engine/scheduler.cpp
    src/engine/model_runner_mock.cpp
    src/engine/model_runner_pto2.cpp
    src/engine/engine.cpp
    src/radix/block_manager.cpp
    src/radix/kv_pool_mock.cpp
    src/sampling/sampler.cpp
    src/frontend/test_path.cpp
    src/pto2/pto2_config.cpp
    src/pto2/pto2_runtime.cpp
)

# Core library
add_library(pypto_core STATIC ${PYPTO_SOURCES})
target_include_directories(pypto_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(pypto_core PUBLIC Threads::Threads)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    # Fetch GTest from GitHub
    message(STATUS "Fetching GTest from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    # Unit tests
    file(GLOB UNIT_TEST_SOURCES tests/unit/*.cpp)
    if(UNIT_TEST_SOURCES)
        add_executable(pypto_unit_tests ${UNIT_TEST_SOURCES})
        target_link_libraries(pypto_unit_tests PRIVATE 
            pypto_core 
            GTest::gtest 
            GTest::gtest_main
        )
        add_test(NAME UnitTests COMMAND pypto_unit_tests)
    endif()
    
    # Integration tests
    file(GLOB INTEGRATION_TEST_SOURCES tests/integration/*.cpp)
    if(INTEGRATION_TEST_SOURCES)
        add_executable(pypto_integration_tests ${INTEGRATION_TEST_SOURCES})
        target_link_libraries(pypto_integration_tests PRIVATE 
            pypto_core 
            GTest::gtest 
            GTest::gtest_main
        )
        add_test(NAME IntegrationTests COMMAND pypto_integration_tests)
    endif()
endif()

# Examples
if(BUILD_EXAMPLES)
    # Mock LLaMA example
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/mock_llama/mock_llama.cpp)
        add_executable(mock_llama examples/mock_llama/mock_llama.cpp)
        target_link_libraries(mock_llama PRIVATE pypto_core)
    endif()
    
    # Simple generate example
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple_generate/simple_generate.cpp)
        add_executable(simple_generate examples/simple_generate/simple_generate.cpp)
        target_link_libraries(simple_generate PRIVATE pypto_core)
    endif()
    
    # PTO2 demo example
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/pto2_demo/pto2_demo.cpp)
        add_executable(pto2_demo examples/pto2_demo/pto2_demo.cpp)
        target_link_libraries(pto2_demo PRIVATE pypto_core)
    endif()
    
    # Simulator demo example
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/simulator_demo/simulator_demo.cpp)
        add_executable(simulator_demo examples/simulator_demo/simulator_demo.cpp)
        target_link_libraries(simulator_demo PRIVATE pypto_core)
    endif()
endif()

# Print configuration
message(STATUS "")
message(STATUS "pypto-serving Configuration:")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "  BUILD_TESTS: ${BUILD_TESTS}")
message(STATUS "  BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "  ENABLE_ASAN: ${ENABLE_ASAN}")
message(STATUS "")
