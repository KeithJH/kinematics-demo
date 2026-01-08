cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 20)

project(FalseSharing CXX)

find_package(benchmark QUIET)
if (NOT benchmark_FOUND)
	include(FetchContent)
	set(BENCHMARK_DOWNLOAD_DEPENDENCIES on)
	FetchContent_Declare(
		benchmark
		GIT_REPOSITORY https://github.com/google/benchmark
		GIT_TAG v1.9.4
	)
	FetchContent_MakeAvailable(benchmark)
endif ()

include(./Common.cmake)

add_executable(bench)
target_link_libraries(bench benchmark::benchmark_main)

add_subdirectory(01_local_points)
add_subdirectory(02_local_atomic_points)
add_subdirectory(03_global_atomic_points)
add_subdirectory(04_global_aligned_points)
add_subdirectory(05_global_aligned_point)
add_subdirectory(06_thread_local_points)
add_subdirectory(07_multiple_arrays)
