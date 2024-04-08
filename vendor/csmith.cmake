# ---------------------------------------------------------------------------
# qcp
# ---------------------------------------------------------------------------

include(ExternalProject)
find_package(Git REQUIRED)

# Get googletest
ExternalProject_Add(
    csmith
    PREFIX "vendor"
    GIT_REPOSITORY "https://github.com/csmith-project/csmith.git"
    GIT_TAG csmith-2.3.0

    INSTALL_DIR "vendor/csmith"
    SOURCE_DIR "vendor/csmith/src"
    TIMEOUT 30
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/vendor/csmith
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    UPDATE_COMMAND ""
)

# Prepare gtest
ExternalProject_Get_Property(csmith install_dir)
set(CSMITH_INCLUDE_DIR ${install_dir}/include)

# set(GTEST_LIBRARY_PATH ${install_dir}/lib/libgtest.a)
file(MAKE_DIRECTORY ${CSMITH_INCLUDE_DIR})

# add_library(gtest STATIC IMPORTED)
# set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY_PATH})
# set_property(TARGET gtest APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})

# add_dependencies(gtest googletest)
