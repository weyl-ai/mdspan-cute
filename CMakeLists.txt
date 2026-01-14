cmake_minimum_required(VERSION 3.21)
project(mdspan_cute_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# mdspan reference implementation (C++23 <mdspan> header)
FetchContent_Declare(
  mdspan
  GIT_REPOSITORY https://github.com/kokkos/mdspan.git
  GIT_TAG stable
)
FetchContent_MakeAvailable(mdspan)

# Catch2 v3
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

# RapidCheck
set(RC_ENABLE_CATCH OFF CACHE BOOL "Disable RapidCheck's bundled Catch2 - we provide Catch2 v3")
FetchContent_Declare(
  rapidcheck
  GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
  GIT_TAG master
)
FetchContent_MakeAvailable(rapidcheck)

# Manually add RapidCheck's Catch2 integration extras
add_library(rapidcheck-catch INTERFACE)
target_include_directories(rapidcheck-catch INTERFACE
  ${rapidcheck_SOURCE_DIR}/extras/catch/include
)
target_link_libraries(rapidcheck-catch INTERFACE rapidcheck Catch2::Catch2)

# Find CUTLASS/cute headers
find_path(CUTLASS_INCLUDE_DIR
  NAMES cute/layout.hpp
  HINTS ENV CUTLASS_DIR
  PATH_SUFFIXES include
  REQUIRED
)
message(STATUS "Found CUTLASS headers at: ${CUTLASS_INCLUDE_DIR}")
include_directories("${CUTLASS_INCLUDE_DIR}")

# Find CUDA CCCL headers (cuda/std/*)
find_path(CUDA_CCCL_INCLUDE_DIR
  NAMES cuda/std/utility
  PATH_SUFFIXES include
  REQUIRED
)
message(STATUS "Found CUDA CCCL headers at: ${CUDA_CCCL_INCLUDE_DIR}")
include_directories("${CUDA_CCCL_INCLUDE_DIR}")

# Find CUDA runtime headers (cuda_runtime_api.h)
# Look for cuda-merged which has all CUDA headers together
find_path(CUDA_RUNTIME_INCLUDE_DIR
  NAMES cuda_runtime_api.h crt/host_defines.h
  HINTS ENV CUDA_HOME
  PATH_SUFFIXES include targets/x86_64-linux/include
  REQUIRED
)
message(STATUS "Found CUDA runtime headers at: ${CUDA_RUNTIME_INCLUDE_DIR}")
include_directories("${CUDA_RUNTIME_INCLUDE_DIR}")

add_library(mdspan_cute INTERFACE)
target_include_directories(mdspan_cute INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(mdspan_cute INTERFACE mdspan::mdspan)

# Example: swizzled tile access
add_executable(swizzled_tile
  examples/swizzled_tile.cpp
)
target_link_libraries(swizzled_tile
  PRIVATE
    mdspan_cute
    mdspan::mdspan
)

add_executable(simulated_1cta_gemm
  examples/simulated_1cta_gemm.cpp
)

target_link_libraries(simulated_1cta_gemm
  PRIVATE
    mdspan_cute
    mdspan::mdspan
)

# Layout bridge tests (requires CUTLASS)
add_executable(layout_cute_tests
  tests/test_layout_cute.cpp
)
target_link_libraries(layout_cute_tests
  PRIVATE
    mdspan_cute
    mdspan::mdspan
    Catch2::Catch2WithMain
    rapidcheck-catch
)

# Property tests (standalone, no CUTLASS needed)
add_executable(property_tests
  tests/property_tests.cpp
)
target_link_libraries(property_tests
  PRIVATE
    mdspan::mdspan
    Catch2::Catch2WithMain
    rapidcheck-catch
)

include(CTest)
include(Catch)
catch_discover_tests(layout_cute_tests)
catch_discover_tests(property_tests)
