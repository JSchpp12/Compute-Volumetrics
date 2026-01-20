#!/bin/sh
set -e

if [[ -z "$VCPKG_ROOT" ]]; then
  echo "Error: VCPKG_ROOT is not set"
  exit 1
fi

export VCPKG_DEFAULT_TRIPLET=x64-linux-dynamic
"$VCPKG_ROOT/vcpkg" install glfw3