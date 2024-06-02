cmake_minimum_required(VERSION 3.15)
project(lua CXX)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_executable(lua lua.cpp)

set_property(TARGET lua PROPERTY CXX_STANDARD 23)

target_link_libraries(lua PRIVATE larley)

add_custom_command(
        TARGET lua POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/test.lua
                ${CMAKE_CURRENT_BINARY_DIR}/test.lua)