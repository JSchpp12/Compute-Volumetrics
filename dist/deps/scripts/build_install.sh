#!/bin/bash
# Configure, build and package the headless Linux release inside the
# rockyLinux_dockerfile image.
set -euo pipefail

# Resolve script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Project root is three levels up from this script
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

: "${VCPKG_ROOT:?VCPKG_ROOT is not set; run this inside the rockyLinux image}"

mkdir -p "${ROOT_DIR}/dist/docker_working"
cd "${ROOT_DIR}/dist/docker_working"

echo "USING $VCPKG_ROOT"
cmake -S "${ROOT_DIR}" -B ./build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DVCPKG_TARGET_TRIPLET=x64-linux-dynamic \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DHEADLESS=ON \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_EXE_LINKER_FLAGS=-latomic

cmake --build ./build

# CMakeLists.txt selects the TGZ generator on Linux, so this writes a .tar.gz.
cd build
cpack .
ls -1 ./*.tar.gz
