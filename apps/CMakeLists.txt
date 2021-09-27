# main
add_executable(main main.cpp)
target_include_directories(main PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(main PRIVATE compiler_flags)
