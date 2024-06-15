# ---------------------------------------------------------------------------
# qcp
# ---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.7)
project(qcp)

# ---------------------------------------------------------------------------
# Environment
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wall -Wextra -Wpedantic -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(SCMAKE_CXX_COMPILER "g++")

# ---------------------------------------------------------------------------
# Packages
# ---------------------------------------------------------------------------

# LLVM
find_package(LLVM REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
llvm_map_components_to_libnames(llvm core)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------
include("${CMAKE_SOURCE_DIR}/clang-tidy.cmake")
include("${CMAKE_SOURCE_DIR}/gperf.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/googletest.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/csmith.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/benchmark.cmake")

# include("${CMAKE_SOURCE_DIR}/vendor/alive2.cmake")

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tools
    ${GTEST_INCLUDE_DIR}
    ${BENCHMARK_INCLUDE_DIR}
)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(INCLUDE_H
    "${CMAKE_SOURCE_DIR}/include/token.h"
    "${CMAKE_SOURCE_DIR}/include/tokenizer.h"
    "${CMAKE_SOURCE_DIR}/include/keywords.h"
    "${CMAKE_SOURCE_DIR}/include/diagnostics.h"
    "${CMAKE_SOURCE_DIR}/include/parser.h"
    "${CMAKE_SOURCE_DIR}/include/operator.h"
    "${CMAKE_SOURCE_DIR}/include/loc.h"
    "${CMAKE_SOURCE_DIR}/include/scope.h"
    "${CMAKE_SOURCE_DIR}/include/type.h"
    "${CMAKE_SOURCE_DIR}/include/basetype.h"
    "${CMAKE_SOURCE_DIR}/include/typefactory.h"
    "${CMAKE_SOURCE_DIR}/include/tracer.h"
    "${CMAKE_SOURCE_DIR}/include/stringpool.h"
    "${CMAKE_SOURCE_DIR}/include/expr.h"
    "${CMAKE_SOURCE_DIR}/include/llvmemitter.h"
    "${CMAKE_SOURCE_DIR}/include/defs/defines.def"
    "${CMAKE_SOURCE_DIR}/include/defs/tokens.def"
    "${CMAKE_SOURCE_DIR}/include/defs/operators.def"
    "${CMAKE_SOURCE_DIR}/include/defs/and_or_operators.def"
    "${CMAKE_SOURCE_DIR}/include/defs/assign_operators.def"
)

set(SRC_CC
    "${CMAKE_SOURCE_DIR}/src/token.cc"
    "${CMAKE_SOURCE_DIR}/src/operator.cc"
    "${CMAKE_SOURCE_DIR}/src/diagnostics.cc"
    "${CMAKE_SOURCE_DIR}/src/stringpool.cc"
    "${CMAKE_SOURCE_DIR}/src/llvmemitter.cc"
)

set(TOOLS_H
    "${CMAKE_SOURCE_DIR}/tools/csmith.h"
)

set(TEST_H

    # "${CMAKE_SOURCE_DIR}/test/test_tokenizer.cc"
    # "${CMAKE_SOURCE_DIR}/test/test_types.cc"
    "${CMAKE_SOURCE_DIR}/test/test_parser.h"
)

set(BENCH_CC
    "${CMAKE_SOURCE_DIR}/bench/bm_tokenizer.cc"
)

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
add_library(libqcp STATIC ${SRC_CC})
target_link_libraries(libqcp ${llvm})

add_executable(qcp src/main.cc)
target_link_libraries(qcp libqcp)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
add_executable(tester test/tester.cc ${TEST_CC})
target_link_libraries(tester libqcp gtest)

enable_testing()
add_test(qcp tester)

# copy test1 folder to build directory
file(COPY ${CMAKE_SOURCE_DIR}/test/test1 DESTINATION ${CMAKE_BINARY_DIR})

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
get_property(BENCHMARK_HEADERS TARGET benchmark PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

add_executable(bench bench/benchmark.cc ${BENCH_CC} ${BENCHMARK_HEADERS})
target_link_libraries(bench libqcp benchmark)

# ---------------------------------------------------------------------------
# Linting
# ---------------------------------------------------------------------------
add_clang_tidy_target(include_linting "${INCLUDE_H}")
add_clang_tidy_target(src_linting "${SRC_CC}")
add_clang_tidy_target(tools_linting "${TOOLS_H}")
add_clang_tidy_target(test_linting "${TEST_H}")
add_clang_tidy_target(bench_linting "${BENCH_CC}")

add_custom_target(lint)
add_dependencies(lint include_linting src_linting test_linting)

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
message(STATUS "[qcp] info")
message(STATUS "    BUILD_TYPE                  = ${CMAKE_BUILD_TYPE}")
message(STATUS "[qcp] settings")
message(STATUS "    GTEST_INCLUDE_DIR           = ${GTEST_INCLUDE_DIR}")
message(STATUS "    GTEST_LIBRARY_PATH          = ${GTEST_LIBRARY_PATH}")
