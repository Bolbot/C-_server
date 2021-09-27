cmake_minimum_required(VERSION 3.13)

project(CPP_server LANGUAGES C;CXX VERSION 0.1)

# interface for flags etc.
add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_11)
set(gcc_compiler_cpp "$<COMPILE_LANG_AND_ID:CXX,AppleClang,ARMClang,Clang,GNU>")
target_compile_options(compiler_flags INTERFACE $<${gcc_compiler_cpp}:-Wall;-Wextra;-Werror;-Wpedantic>)

add_subdirectory(src)		# server, utils, multithreading
add_subdirectory(apps)		# main

target_link_libraries(main PRIVATE utils server)
