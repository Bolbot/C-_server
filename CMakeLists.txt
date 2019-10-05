cmake_minimum_required(VERSION 3.0)

project(mailru CXX)
set(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra")

find_package(Threads)
find_package(Boost 1.65 REQUIRED COMPONENTS program_options)
add_library(utils source/utils.cpp)
add_library(server source/server.cpp)
add_library(multithreading source/multithreading.cpp)
add_executable(main source/main.cpp)

target_link_libraries(utils ${Boost_LIBRARIES})
target_link_libraries(server multithreading)
target_link_libraries(main utils server ${CMAKE_THREAD_LIBS_INIT})
