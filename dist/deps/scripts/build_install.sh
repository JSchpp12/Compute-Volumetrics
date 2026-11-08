#!/bin/bash
# Configure, build and package headless Linux tarballs inside the
# rockyLinux_dockerfile image.
#
# Usage: build_install.sh [Release|Debug]... [--compiler clang|gcc]
# With no build type a Release build is produced; the compiler defaults to
# clang.
set -euo pipefail

# Resolve script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Project root is three levels up from this script
ROOT_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

: "${VCPKG_ROOT:?VCPKG_ROOT is not set; run this inside the rockyLinux image}"

BUILD_TYPES=""
COMPILER="clang"

add_build_type() {
    case " ${BUILD_TYPES} " in
        *" $1 "*) ;;
        *) BUILD_TYPES="${BUILD_TYPES} $1" ;;
    esac
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --compiler)
            if [ "$#" -lt 2 ]; then
                echo "error: --compiler requires a value (clang|gcc)" >&2
                exit 1
            fi
            COMPILER="$2"
            shift 2
            ;;
        --compiler=*)
            COMPILER="${1#*=}"
            shift
            ;;
        *)
            # Normalize so "debug", "Debug", and "--debug" all work.
            normalized="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"
            normalized="${normalized#--}"
            case "${normalized}" in
                release)  add_build_type Release ;;
                debug)    add_build_type Debug ;;
                both|all) add_build_type Release; add_build_type Debug ;;
                *)
                    echo "error: unsupported build type '$1' (expected Release or Debug)" >&2
                    exit 1
                    ;;
            esac
            shift
            ;;
    esac
done

COMPILER="$(printf '%s' "${COMPILER}" | tr '[:upper:]' '[:lower:]')"
case "${COMPILER}" in
    clang) CC=clang; CXX=clang++ ;;
    gcc)   CC=gcc;   CXX=g++ ;;
    *)
        echo "error: unsupported compiler '${COMPILER}' (expected clang or gcc)" >&2
        exit 1
        ;;
esac

# vcpkg's native Linux toolchain does not inherit the outer CMake compiler, so
# the export is what makes the dependency ports build with the same toolchain
# instead of silently falling back to the distro default.
export CC CXX

if [ -z "${BUILD_TYPES}" ]; then
    BUILD_TYPES="Release"
fi

# shellcheck disable=SC2086  # intentional word splitting of the build type list
for BUILD_TYPE in ${BUILD_TYPES}; do

    # Keep each compiler/variant pair in its own build tree so the CMake cache
    # and packaged tarball from one cannot leak into another.  The compiler is
    # part of the path because CMake refuses to reuse a tree configured with a
    # different toolchain.
    WORK_DIR="${ROOT_DIR}/dist/docker_working/${COMPILER}/$(printf '%s' "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')"
    mkdir -p "${WORK_DIR}"
    cd "${WORK_DIR}"

    echo "=== Building ${BUILD_TYPE} headless with ${COMPILER} ==="
    echo "USING $VCPKG_ROOT"
    "${CXX}" --version | head -n 1

    # The packaging triplet (dist/deps/triplets) is release-only, so a Debug
    # build links against the release dependency libraries.  The application
    # itself is still compiled with -g and without NDEBUG.
    cmake -S "${ROOT_DIR}" -B ./build -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DVCPKG_TARGET_TRIPLET=x64-linux-dynamic-release \
        -DVCPKG_OVERLAY_TRIPLETS="${ROOT_DIR}/dist/deps/triplets" \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DHEADLESS=ON \
        -DCMAKE_C_COMPILER="${CC}" \
        -DCMAKE_CXX_COMPILER="${CXX}" \
        -DCMAKE_EXE_LINKER_FLAGS=-latomic

    cmake --build ./build

    # CMakeLists.txt selects the TGZ generator on Linux, so this writes a .tar.gz.
    cd build
    cpack .
    ls -1 ./*.tar.gz
    cd "${ROOT_DIR}"
done
