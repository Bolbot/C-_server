cmake_minimum_required(VERSION 2.8)

project(mailru CXX)
#especially for mail.ru archaic versions of CMake and Boost, name final, -lpthread
set(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra -std=c++11 -lpthread")

find_package(Threads)
find_package(Boost 1.54 REQUIRED COMPONENTS program_options)
add_library(utils utils.cpp)
add_library(server server.cpp)
add_library(multithreading multithreading.cpp)
add_executable(final main.cpp)

target_link_libraries(utils ${Boost_LIBRARIES})
target_link_libraries(server multithreading)
target_link_libraries(final utils server ${CMAKE_THREAD_LIBS_INIT})
