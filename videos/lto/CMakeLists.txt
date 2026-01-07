cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(CompareLto CXX)

find_package(benchmark QUIET)
if (NOT benchmark_FOUND)
	include(FetchContent)
	set(BENCHMARK_DOWNLOAD_DEPENDENCIES on)
	FetchContent_Declare(
		benchmark
		GIT_REPOSITORY https://github.com/google/benchmark
		GIT_TAG v1.9.4
		SYSTEM
	)
	FetchContent_MakeAvailable(benchmark)
endif ()

include(./Common.cmake)
add_executable(default)
target_link_libraries(default benchmark::benchmark_main)

add_executable(lto)
target_link_libraries(lto benchmark::benchmark_main)
set_property(TARGET lto PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

add_subdirectory(01_aos)
add_subdirectory(02_large_structure)
add_subdirectory(03_soa)
add_subdirectory(04_hybrid)
add_subdirectory(05_aligned)
