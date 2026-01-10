#!/bin/sh
set -e

if [[ -z "$VCPKG_ROOT" ]]; then
  echo "Error: VCPKG_ROOT is not set"
  exit 1
fi

"$VCPKG_ROOT/vcpkg" install abseil nlohmann-json stb blosc glm vulkan-memory-allocator tbb basisu tinyobjloader boost-thread boost-lockfree boost-container boost-filesystem boost-log ktx[vulkan] gdal[png,geos] openvdb[nanovdb,nanovdb-tools]