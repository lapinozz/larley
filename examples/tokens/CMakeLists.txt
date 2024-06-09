cmake_minimum_required(VERSION 3.15)
project(tokens CXX)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(tokens tokens.cpp)

set_property(TARGET tokens PROPERTY CXX_STANDARD 23)

target_link_libraries(tokens PRIVATE larley)