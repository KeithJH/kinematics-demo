add_library(${PROJECT_NAME} Simulation.cpp VectorOfStructSim.cpp StructOfVectorSim.cpp StructOfArraySim.cpp)
target_link_libraries(${PROJECT_NAME} raylib)
target_compile_options(${PROJECT_NAME} PRIVATE ${WARNING_OPTIONS} ${SANITIZER_OPTIONS})
target_link_options(${PROJECT_NAME} PRIVATE ${SANITIZER_OPTIONS})
