#!/bin/sh
set -e

if [[ -z "$VCPKG_ROOT" ]]; then
  echo "Error: VCPKG_ROOT is not set"
  exit 1
fi

export VCPKG_DEFAULT_TRIPLET=x64-linux-dynamic
"$VCPKG_ROOT/vcpkg" install tbb usd openvdb[nanovdb,nanovdb-tools] abseil nlohmann-json stb blosc glm vulkan-memory-allocator basisu tinyobjloader boost-thread boost-lockfree boost-container boost-filesystem boost-log ktx[vulkan] gdal[png,geos]