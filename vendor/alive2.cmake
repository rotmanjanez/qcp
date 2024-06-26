# ---------------------------------------------------------------------------
# qcp
# ---------------------------------------------------------------------------

include(ExternalProject)
find_package(Git REQUIRED)

# Get alive2
ExternalProject_Add(
    alive2
    PREFIX "vendor"
    GIT_REPOSITORY "https://github.com/AliveToolkit/alive2.git"
    GIT_SHALLOW ON
    GIT_PROGRESS ON

    INSTALL_DIR "vendor/alive2"
    SOURCE_DIR "vendor/alive2/src"
    TIMEOUT 30
    CMAKE_ARGS
    -DCMAKE_PREFIX_PATH=${CMAKE_BINARY_DIR}/vendor/llvm
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/vendor/alive2
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_TV=ON
    -DLLVM_DIR=${CMAKE_BINARY_DIR}/vendor/llvm/lib/cmake/llvm
    UPDATE_COMMAND ""
)

# Prepare gtest
# ExternalProject_Get_Property(alive2 install_dir)
# set(CSMITH_INCLUDE_DIR ${install_dir}/include)

# set(GTEST_LIBRARY_PATH ${install_dir}/lib/libgtest.a)
# file(MAKE_DIRECTORY ${CSMITH_INCLUDE_DIR})

# add_library(gtest STATIC IMPORTED)
# set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY_PATH})
# set_property(TARGET gtest APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})

# add_dependencies(gtest googletest)
