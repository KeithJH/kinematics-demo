cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(FalseSharing CXX)
include(./Common.cmake)

find_package(benchmark REQUIRED)

add_executable(bench)
target_link_libraries(bench benchmark::benchmark_main)

add_subdirectory(01_local_points)
add_subdirectory(02_local_atomic_points)
