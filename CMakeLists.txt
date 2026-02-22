cmake_minimum_required(VERSION 3.20)
project(arena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(arena INTERFACE)
add_library(arena::arena ALIAS arena)

target_include_directories(arena INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if (MSVC)
  target_compile_options(arena INTERFACE /W4 /permissive-)
else()
  target_compile_options(arena INTERFACE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
enable_testing()

add_executable(arena_basic_test tests/test_basic.cpp)
target_link_libraries(arena_basic_test PRIVATE arena::arena)
add_test(NAME arena.basic COMMAND arena_basic_test)
