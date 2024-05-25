cmake_minimum_required(VERSION 3.15)
project(json CXX)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(json json.cpp)

set_property(TARGET json PROPERTY CXX_STANDARD 23)

target_link_libraries(json PRIVATE larley)