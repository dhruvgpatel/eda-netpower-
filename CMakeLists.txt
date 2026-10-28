cmake_minimum_required(VERSION 3.20)
project(netpower VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure executable output path is consistent for scripts and CI
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Optional Sanitizers
option(NETPOWER_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(NETPOWER_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    else()
        message(WARNING "Sanitizers requested but compiler ${CMAKE_CXX_COMPILER_ID} is not Clang/GCC")
    endif()
endif()

# Core Netpower Library
add_library(netpower_core
    src/circuit.cpp
    src/parser.cpp
    src/graph.cpp
    src/simulator.cpp
    src/power.cpp
    src/report.cpp
    src/cli.cpp
)

target_include_directories(netpower_core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Netpower CLI Executable
add_executable(netpower src/main.cpp)
target_link_libraries(netpower PRIVATE netpower_core)

# Testing
include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

