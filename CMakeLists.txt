cmake_minimum_required(VERSION 3.25)

# Generate compile_commands.json
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
include(FetchContent)

## Raylib
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    SYSTEM
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
  endif()
endif()

## Catch2
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.0
  SYSTEM
)
FetchContent_MakeAvailable(Catch2)

# Our Project
project(kinematics-demo CXX)

include("cmake/Warnings.gcc.cmake")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  include("cmake/Sanitizers.gcc.cmake")
endif ()

add_executable(${PROJECT_NAME}-gui kinematics.cpp main.cpp)
target_link_libraries(${PROJECT_NAME}-gui raylib)
target_compile_options(${PROJECT_NAME}-gui PRIVATE ${WARNING_OPTIONS} ${SANITIZER_OPTIONS})
target_link_options(${PROJECT_NAME}-gui PRIVATE ${SANITIZER_OPTIONS})
