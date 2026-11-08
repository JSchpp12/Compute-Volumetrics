#!/usr/bin/env bash
# Build the portability image and produce headless tarballs.
#
# Usage: create_install_tar_rockyLinux.sh [release|debug|both] [--compiler clang|gcc]
#   release    headless Release tarball (default)
#   debug      headless Debug tarball with symbols
#   both       both variants
#   --compiler clang (default) or gcc (legacy toolchain)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

usage() {
    cat <<'EOF'
Usage: create_install_tar_rockyLinux.sh [release|debug|both] [--compiler clang|gcc]

  release     Build and package the headless Release tarball (default).
  debug       Build and package the headless Debug tarball with symbols.
  both        Build and package both variants.
  --compiler  Toolchain used for every requested variant: clang (default) or
              gcc (legacy).
EOF
}

BUILD_TYPES=""
COMPILER="clang"

add_build_type() {
    case " ${BUILD_TYPES} " in
        *" $1 "*) ;;
        *) BUILD_TYPES="${BUILD_TYPES} $1" ;;
    esac
}

set_compiler() {
    case "$1" in
        clang|gcc) COMPILER="$1" ;;
        *)
            echo "error: unsupported compiler '$1' (expected clang or gcc)" >&2
            usage >&2
            exit 1
            ;;
    esac
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --compiler)
            if [ "$#" -lt 2 ]; then
                echo "error: --compiler requires a value (clang|gcc)" >&2
                usage >&2
                exit 1
            fi
            set_compiler "$(printf '%s' "$2" | tr '[:upper:]' '[:lower:]')"
            shift 2
            ;;
        --compiler=*)
            set_compiler "$(printf '%s' "${1#*=}" | tr '[:upper:]' '[:lower:]')"
            shift
            ;;
        *)
            # Normalize so "debug", "Debug", and "--debug" all work.
            normalized="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"
            normalized="${normalized#--}"
            case "${normalized}" in
                release)  add_build_type Release; shift ;;
                debug)    add_build_type Debug; shift ;;
                both|all) add_build_type Release; add_build_type Debug; shift ;;
                -h|help)  usage; exit 0 ;;
                *)        echo "error: unknown argument '$1'" >&2; usage >&2; exit 1 ;;
            esac
            ;;
    esac
done

if [ -z "${BUILD_TYPES}" ]; then
    BUILD_TYPES="Release"
fi

docker build -f "${SCRIPT_DIR}/deps/rockyLinux_dockerfile" -t starlight:rockyLinux "${SCRIPT_DIR}/deps"

echo "Repository root: ${PARENT_DIR}"
echo "Build variant(s):${BUILD_TYPES}"
echo "Compiler: ${COMPILER}"
# shellcheck disable=SC2086  # intentional word splitting of the build type list
docker run --rm \
    -v "${PARENT_DIR}:/app" \
    -w /app \
    starlight:rockyLinux \
    /bin/bash ./dist/deps/scripts/build_install.sh --compiler "${COMPILER}" ${BUILD_TYPES}
