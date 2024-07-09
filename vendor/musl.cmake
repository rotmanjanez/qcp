# ---------------------------------------------------------------------------
# qcp
# ---------------------------------------------------------------------------

include(ExternalProject)
find_package(Git REQUIRED)

# Get alive2
ExternalProject_Add(
    musl
    PREFIX "vendor"
    GIT_REPOSITORY "https://git.musl-libc.org/git/musl"
    GIT_SHALLOW ON
    GIT_PROGRESS ON

    TIMEOUT 30
    SOURCE_DIR "vendor/musl"
    CONFIGURE_COMMAND "${CMAKE_BINARY_DIR}/vendor/musl/configure" "--prefix=${CMAKE_BINARY_DIR}/vendor/musl/build"
    BUILD_COMMAND "make" "-j${NPROC}" "install"
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)