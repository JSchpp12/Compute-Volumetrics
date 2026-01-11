#!/bin/bash
# Resolve script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Project root assumed two levels up from this script; adjust if needed
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"

mkdir -p dist/docker_working
cd extern/StarlightAppBuilder
/bin/bash ./init.sh

cd ../../dist
/bin/bash ./deps/scripts/install_deps_vcpkg.sh

cd docker_working
echo "USING $VCPKG_ROOT"
cmake -S ../../ -B ./build -G Ninja -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DHEADLESS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXE_LINKER_FLAGS=-latomic -DCPACK_GENERATOR=TGZ
cd build
cmake --build . --config Release
cpack .

#then export out the package