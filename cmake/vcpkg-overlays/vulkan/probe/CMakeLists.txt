cmake_minimum_required(VERSION 3.25)

project(find_vulkan LANGUAGES C)

# Locate the system Vulkan SDK. $VULKAN_SDK is forwarded to vcpkg port builds,
# so CMake's FindVulkan will use the installed SDK when one is present.
find_package(Vulkan QUIET)

set(OUTFILE "${OUTFILE}" CACHE FILEPATH "")
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/vulkan-result.cmake.in"
    "${OUTFILE}"
    @ONLY
    ESCAPE_QUOTES
)
