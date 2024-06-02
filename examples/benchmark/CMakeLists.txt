cmake_minimum_required(VERSION 3.15)
project(benchmark CXX)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(benchmark benchmark.cpp)

set_property(TARGET benchmark PROPERTY CXX_STANDARD 23)

target_link_libraries(benchmark PRIVATE larley)