#!/bin/sh
set -e

if [[ -z "$VCPKG_ROOT" ]]; then
  echo "Error: VCPKG_ROOT is not set"
  exit 1
fi

# Release-only triplet: dependency debug libraries are not built/installed.
# The triplet lives in the repo's dist/deps/triplets overlay.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
export VCPKG_OVERLAY_TRIPLETS="${SCRIPT_DIR}/../triplets"
export VCPKG_DEFAULT_TRIPLET=x64-linux-dynamic-release
"$VCPKG_ROOT/vcpkg" install tbb openvdb[nanovdb,nanovdb-tools] abseil nlohmann-json stb blosc glm vulkan-memory-allocator basisu tinyobjloader boost-thread boost-lockfree boost-container boost-filesystem boost-log ktx[vulkan] gdal[png,geos] gtest