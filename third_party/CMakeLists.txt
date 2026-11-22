# Scoped dynarmic configuration (isolated from Hikari targets).
include(FetchContent)

# Provide a newer pinned fmt (11.1.4) so compile-time format checking remains enabled
# under Clang 22.1.8 (the fmt bundled in dynarmic fails consteval evaluation in os.cc/format-inl.h).
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
set(FMT_INSTALL ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 123913715afeb8a437e6388b4473fcc4753e1c9a
)
FetchContent_MakeAvailable(fmt)

# Copy bundled mcl to the build tree and apply our build-time patch without modifying tracked files.
set(_mcl_src_dir "${CMAKE_CURRENT_BINARY_DIR}/mcl-src")
set(_mcl_bin_dir "${CMAKE_CURRENT_BINARY_DIR}/mcl-build")
file(REMOVE_RECURSE "${_mcl_src_dir}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/dynarmic/externals/mcl/" DESTINATION "${_mcl_src_dir}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env GIT_DIR=/nonexistent git apply
        "${CMAKE_CURRENT_SOURCE_DIR}/patches/0001-mcl-lift-sequence-integer-sequence.patch"
    WORKING_DIRECTORY "${_mcl_src_dir}"
    RESULT_VARIABLE _mcl_patch_rc
)
if(NOT _mcl_patch_rc EQUAL 0)
    message(FATAL_ERROR "Failed to apply mcl lift_sequence.hpp patch (exit ${_mcl_patch_rc}).")
endif()
set(MCL_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory("${_mcl_src_dir}" "${_mcl_bin_dir}")

set(DYNARMIC_TESTS OFF CACHE BOOL "" FORCE)
set(DYNARMIC_USE_BUNDLED_EXTERNALS ON CACHE BOOL "" FORCE)
set(DYNARMIC_FRONTENDS "A64" CACHE STRING "" FORCE)
set(DYNARMIC_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Scoped to third_party/dynarmic only: externals/robin-map/CMakeLists.txt specifies
# cmake_minimum_required(VERSION 3.1), which CMake 4.x rejects without this policy minimum.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
add_subdirectory(dynarmic)
unset(CMAKE_POLICY_VERSION_MINIMUM)
