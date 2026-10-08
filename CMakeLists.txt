# SPDX-FileCopyrightText: 2026 ccontino84
# SPDX-License-Identifier: LicenseRef-TBD

cmake_minimum_required(VERSION 3.16)
project(parrotfish-keyboard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(PARROTFISH_TESTS "Build the host tests and benchmarks" ON)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_subdirectory(engine)
add_subdirectory(data)

if(PARROTFISH_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
