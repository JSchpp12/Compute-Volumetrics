# Overlay for the vcpkg "vulkan" stub port.
#
# The registry port unconditionally depends on vulkan-headers and
# vulkan-loader, which makes vcpkg download and build a second copy of the
# Vulkan SDK even when one is already installed on the machine.
#
# This overlay deliberately does not depend on the Vulkan packages. It only
# verifies that a system Vulkan SDK can be located (through $VULKAN_SDK /
# CMake's FindVulkan), so downstream ports (imgui's vulkan-binding) and the
# main project can use the SDK that is already present. The only dependency is
# the "vcpkg-cmake" host helper used below.

set(VCPKG_POLICY_EMPTY_PACKAGE enabled)

set(_vulkan_result_file
    "${CURRENT_BUILDTREES_DIR}/vulkan-detect-${TARGET_TRIPLET}.cmake.log"
)

vcpkg_cmake_configure(
    SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}/probe"
    OPTIONS
        "-DOUTFILE=${_vulkan_result_file}"
)

include("${_vulkan_result_file}")

if(NOT DETECTED_Vulkan_FOUND)
    message(FATAL_ERROR
        "A Vulkan SDK was not found on this machine.\n"
        "This project uses the Vulkan SDK that is installed on the system and "
        "does not download one through vcpkg.\n"
        "Please install the Vulkan SDK (https://vulkan.lunarg.com/sdk/home) or "
        "set the VULKAN_SDK environment variable so CMake can locate it."
    )
endif()

message(STATUS
    "Using system Vulkan ${DETECTED_Vulkan_VERSION} "
    "(headers: ${DETECTED_Vulkan_INCLUDE_DIRS}, loader: ${DETECTED_Vulkan_LIBRARY})"
)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright"
    "Stub package. This overlayed port uses the Vulkan SDK already installed on the system.\n")
