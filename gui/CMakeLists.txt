add_executable(${PROJECT_NAME}-gui App.cpp main.cpp)
target_link_libraries(${PROJECT_NAME}-gui ${PROJECT_NAME} raylib)
target_compile_options(${PROJECT_NAME}-gui PRIVATE ${WARNING_OPTIONS} ${SANITIZER_OPTIONS})
target_link_options(${PROJECT_NAME}-gui PRIVATE ${SANITIZER_OPTIONS})
