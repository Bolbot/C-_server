# multithreading
find_package(Threads REQUIRED)
add_library(multithreading multithreading.cpp)
target_include_directories(multithreading PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(multithreading PRIVATE ${CMAKE_THREAD_LIBS_INIT} compiler_flags)

# utils
find_package(Boost REQUIRED COMPONENTS program_options)
add_library(utils utils.cpp)
target_include_directories(utils PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(utils PRIVATE Boost::program_options multithreading compiler_flags)

# server
add_library(server server.cpp)
target_include_directories(server PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(server PRIVATE compiler_flags)
