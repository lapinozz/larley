cmake_minimum_required(VERSION 3.15)

project(larley CXX)

add_library(larley INTERFACE)

set_property(TARGET larley PROPERTY CXX_STANDARD 23)

if(CMAKE_GENERATOR STREQUAL "Ninja")
  # gcc/clang won't emit color codes if the output medium isn't a terminal
  # ninja interferes with this => force colored output
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options (-fdiagnostics-color=always)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options (-fcolor-diagnostics)
  endif()
endif()

target_include_directories(larley INTERFACE 
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>)

option(ENABLE_EXAMPLE "Enable examples" ON)

if(ENABLE_EXAMPLE)
  add_subdirectory(examples)
endif()

## headers
set_target_properties(larley PROPERTIES PUBLIC_HEADER "include/larley")
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/
        DESTINATION include)