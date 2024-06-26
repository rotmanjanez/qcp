# ---------------------------------------------------------------------------
# qcp
# ---------------------------------------------------------------------------

include(ExternalProject)
find_package(Git REQUIRED)

# Get googletest
ExternalProject_Add(
    googletest
    PREFIX "vendor/gtm"
    GIT_REPOSITORY "https://github.com/google/googletest.git"
    GIT_TAG release-1.8.0
    GIT_SHALLOW ON
    GIT_PROGRESS ON
    INSTALL_DIR "vendor/gtm/gtest"
    SOURCE_DIR "vendor/gtm/src/googletest/googletest"
    TIMEOUT 30
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/vendor/gtm/gtest
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
    UPDATE_COMMAND ""
    BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libgtest.a
)

# Prepare gtest
ExternalProject_Get_Property(googletest install_dir)
set(GTEST_INCLUDE_DIR ${install_dir}/include)
set(GTEST_LIBRARY_PATH ${install_dir}/lib/libgtest.a)
file(MAKE_DIRECTORY ${GTEST_INCLUDE_DIR})
add_library(gtest STATIC IMPORTED)
set_property(TARGET gtest PROPERTY IMPORTED_LOCATION ${GTEST_LIBRARY_PATH})
set_property(TARGET gtest APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIR})

add_dependencies(gtest googletest)
