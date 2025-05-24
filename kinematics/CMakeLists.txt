find_package(OpenMP)

add_library(${PROJECT_NAME} Simulation.cpp VectorOfStructSim.cpp StructOfVectorSim.cpp StructOfArraySim.cpp StructOfPointerSim.cpp StructOfAlignedSim.cpp StructOfOversizedSim.cpp OmpSimdSim.cpp OmpForSim.cpp ShaderSim.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include/)

target_link_libraries(${PROJECT_NAME} raylib OpenMP::OpenMP_CXX)
target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_OPTIONS} ${SANITIZER_OPTIONS})
target_link_options(${PROJECT_NAME} PRIVATE ${SANITIZER_OPTIONS})
