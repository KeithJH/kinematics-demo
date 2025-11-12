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
add_subdirectory(03_global_atomic_points)
add_subdirectory(04_global_aligned_points)
add_subdirectory(05_global_aligned_point)
add_subdirectory(06_thread_local_points)
add_subdirectory(07_multiple_arrays)
