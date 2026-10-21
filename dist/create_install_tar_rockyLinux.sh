#!/usr/bin/env bash
# Build the portability image and produce the headless release tarball.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

docker build -f "${SCRIPT_DIR}/deps/rockyLinux_dockerfile" -t starlight:rockyLinux "${SCRIPT_DIR}/deps"

echo "Repository root: ${PARENT_DIR}"
docker run --rm \
    -v "${PARENT_DIR}:/app" \
    -w /app \
    starlight:rockyLinux \
    /bin/bash ./dist/deps/scripts/build_install.sh
