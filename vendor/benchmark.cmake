# ---------------------------------------------------------------------------
# wcp
# ---------------------------------------------------------------------------

include(ExternalProject)
find_package(Git REQUIRED)

# Get gflags
ExternalProject_Add(
        benchmark_src
        PREFIX "vendor/benchmark"
        GIT_REPOSITORY "https://github.com/google/benchmark.git"
        GIT_TAG v1.8.3
        GIT_SHALLOW ON
        GIT_PROGRESS ON

        TIMEOUT 10
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/vendor/benchmark
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS_RELEASE}
        -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
        -DBENCHMARK_ENABLE_LIBPFM=${PERF_COUNTERS_ENABLED}
        UPDATE_COMMAND ""
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libbenchmark.a
)

# Prepare
ExternalProject_Get_Property(benchmark_src install_dir)
set(BENCHMARK_INCLUDE_DIR ${install_dir}/include)
set(BENCHMARK_LIBRARY_PATH ${install_dir}/lib/libbenchmark.a)
file(MAKE_DIRECTORY ${BENCHMARK_INCLUDE_DIR})
add_library(benchmark STATIC IMPORTED)
set_property(TARGET benchmark PROPERTY IMPORTED_LOCATION ${BENCHMARK_LIBRARY_PATH})
set_property(TARGET benchmark APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${BENCHMARK_INCLUDE_DIR})

# Dependencies
add_dependencies(benchmark benchmark_src)
