cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CompareLto CXX)
include(./Common.cmake)

find_package(benchmark REQUIRED)

add_executable(default)
target_link_libraries(default benchmark::benchmark_main)

add_executable(lto)
target_link_libraries(lto benchmark::benchmark_main)
set_property(TARGET lto PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

add_subdirectory(01_aos)
add_subdirectory(02_large_structure)
