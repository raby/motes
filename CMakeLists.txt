cmake_minimum_required(VERSION 3.14)
project(motes VERSION 0.1.0 LANGUAGES CXX
        DESCRIPTION "Small, fast, dependency-free connected-components / blob detection")

# Header-only interface library: consumers just add the include directory.
add_library(motes INTERFACE)
add_library(motes::motes ALIAS motes)
target_include_directories(motes INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(motes INTERFACE cxx_std_17)

# Build the tests when motes is the top-level project; off when added as a subproject.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(MOTES_BUILD_TESTS "Build the motes tests" ON)
else()
  option(MOTES_BUILD_TESTS "Build the motes tests" OFF)
endif()

if(MOTES_BUILD_TESTS)
  enable_testing()
  add_executable(motes_tests tests/test_motes.cpp)
  target_link_libraries(motes_tests PRIVATE motes)
  target_include_directories(motes_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests) # vendored doctest.h
  target_compile_features(motes_tests PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(motes_tests PRIVATE /W4)
  else()
    target_compile_options(motes_tests PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  add_test(NAME motes_tests COMMAND motes_tests)
endif()

# Benchmark: a standalone, optimised executable. Built (so CI keeps it compiling) but not run by
# ctest — benchmarks are noisy. Run it by hand: ./build/motes_bench
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  option(MOTES_BUILD_BENCH "Build the motes benchmark" ON)
else()
  option(MOTES_BUILD_BENCH "Build the motes benchmark" OFF)
endif()

if(MOTES_BUILD_BENCH)
  add_executable(motes_bench bench/bench.cpp)
  target_link_libraries(motes_bench PRIVATE motes)
  target_compile_features(motes_bench PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(motes_bench PRIVATE /O2)
  else()
    target_compile_options(motes_bench PRIVATE -O3)
  endif()
endif()
