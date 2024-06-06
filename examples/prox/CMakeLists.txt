cmake_minimum_required(VERSION 3.15)
project(prox CXX)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(prox prox.cpp)

set_property(TARGET prox PROPERTY CXX_STANDARD 23)

target_link_libraries(prox PRIVATE larley)