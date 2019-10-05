cmake_minimum_required(VERSION 2.8)

project(mailru CXX)
#especially for mail.ru archaic versions of CMake and Boost, name final, -pthread
set(CMAKE_CXX_FLAGS "-Wall -Werror -Wextra -std=c++11 -pthread")

#an attempt of workaround, because mailru machine cannot find it's own pthread
set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(CMAKE_HAVE_THREADS_LIBRARY 1)
set(CMAKE_USE_WIN32_THREADS_INIT 0)
set(CMAKE_USE_PTHREADS_INIT 1)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads)
find_package(Boost 1.54 REQUIRED COMPONENTS program_options)
add_library(utils utils.cpp)
add_library(server server.cpp)
add_library(multithreading multithreading.cpp)
add_executable(final main.cpp)

target_link_libraries(utils ${Boost_LIBRARIES})
target_link_libraries(multithreading ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(server multithreading)
target_link_libraries(final utils server)
