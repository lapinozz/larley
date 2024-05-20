cmake_minimum_required(VERSION 3.15)
project(main-dev CXX)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(main-dev main-dev.cpp)

set_property(TARGET main-dev PROPERTY CXX_STANDARD 23)

target_link_libraries(main-dev PRIVATE larley)