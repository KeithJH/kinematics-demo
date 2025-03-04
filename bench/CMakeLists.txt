add_executable(${PROJECT_NAME}-bench main.cpp)
target_link_libraries(${PROJECT_NAME}-bench raylib Catch2::Catch2WithMain)
target_compile_options(${PROJECT_NAME}-bench PRIVATE ${WARNING_OPTIONS} ${SANITIZER_OPTIONS})
target_link_options(${PROJECT_NAME}-bench PRIVATE ${SANITIZER_OPTIONS})
