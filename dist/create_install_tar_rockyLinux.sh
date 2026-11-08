#!/usr/bin/env bash
# Build the portability image and produce headless tarballs.
#
# Usage: create_install_tar_rockyLinux.sh [release|debug|both]
#        [--compiler clang|gcc] [--rebuild-image]
#   release    headless Release tarball (default)
#   debug      headless Debug tarball with symbols
#   both       both variants
#   --compiler clang (default) or gcc (legacy toolchain)
#   --rebuild-image
#              rebuild the rockyLinux image even if the existing one was
#              built from the current Dockerfile
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

usage() {
    cat <<'EOF'
Usage: create_install_tar_rockyLinux.sh [release|debug|both] [--compiler clang|gcc] [--rebuild-image]

  release         Build and package the headless Release tarball (default).
  debug           Build and package the headless Debug tarball with symbols.
  both            Build and package both variants.
  --compiler      Toolchain used for every requested variant: clang (default)
                  or gcc (legacy).
  --rebuild-image Rebuild the rockyLinux image even if the existing one was
                  built from the current Dockerfile.  Without it the image is
                  only (re)built when it is missing or stale.
EOF
}

BUILD_TYPES=""
COMPILER="clang"
REBUILD_IMAGE=0

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
        --rebuild-image)
            REBUILD_IMAGE=1
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

# ---------------------------------------------------------------------------
# Image build, skipped when the existing image already matches the Dockerfile.
# ---------------------------------------------------------------------------
IMAGE_NAME="starlight:rockyLinux"
DOCKERFILE="${SCRIPT_DIR}/deps/rockyLinux_dockerfile"
BUILD_CONTEXT="${SCRIPT_DIR}/deps"
# Label stamped on the image at build time with the Dockerfile's hash.
IMAGE_LABEL_KEY="starlight.dockerfile.sha256"

# The Dockerfile has no COPY/ADD, so its contents alone determine the image;
# the rest of the build context does not need to be hashed.
hash_file() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    elif command -v openssl >/dev/null 2>&1; then
        openssl dgst -sha256 "$1" | awk '{print $NF}'
    else
        return 1
    fi
}

# Without a hashing tool there is no way to tell whether the image is current,
# so fall back to always building.
DOCKERFILE_HASH="$(hash_file "${DOCKERFILE}")" || DOCKERFILE_HASH=""

image_is_current() {
    [ -n "${DOCKERFILE_HASH}" ] || return 1
    docker image inspect "${IMAGE_NAME}" >/dev/null 2>&1 || return 1
    current_label="$(docker image inspect \
        --format "{{ index .Config.Labels \"${IMAGE_LABEL_KEY}\" }}" \
        "${IMAGE_NAME}" 2>/dev/null || true)"
    [ "${current_label}" = "${DOCKERFILE_HASH}" ]
}

if [ "${REBUILD_IMAGE}" -eq 0 ] && image_is_current; then
    echo "Reusing existing image ${IMAGE_NAME} (matches ${DOCKERFILE##*/}; pass --rebuild-image to force a rebuild)"
else
    if [ "${REBUILD_IMAGE}" -eq 1 ]; then
        echo "Rebuilding image ${IMAGE_NAME} (--rebuild-image)"
    else
        echo "Building image ${IMAGE_NAME} from ${DOCKERFILE}"
    fi
    LABEL_ARG=""
    if [ -n "${DOCKERFILE_HASH}" ]; then
        LABEL_ARG="--label ${IMAGE_LABEL_KEY}=${DOCKERFILE_HASH}"
    fi
    # shellcheck disable=SC2086  # LABEL_ARG is empty or exactly one arg pair
    docker build -f "${DOCKERFILE}" -t "${IMAGE_NAME}" ${LABEL_ARG} "${BUILD_CONTEXT}"
fi

echo "Repository root: ${PARENT_DIR}"
echo "Build variant(s):${BUILD_TYPES}"
echo "Compiler: ${COMPILER}"
# shellcheck disable=SC2086  # intentional word splitting of the build type list
docker run --rm \
    -v "${PARENT_DIR}:/app" \
    -w /app \
    starlight:rockyLinux \
    /bin/bash ./dist/deps/scripts/build_install.sh --compiler "${COMPILER}" ${BUILD_TYPES}
