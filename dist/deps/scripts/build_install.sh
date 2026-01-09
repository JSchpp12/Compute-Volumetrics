#!/bin/bash 
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PARENT_DIR="$(dirname "$SCRIPT_DIR")"

mkdir -p dist/docker_working
cd dist/docker_working
if [[ ! -d "vcpkg" ]]; then
    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.sh
fi

cd ../../extern/StarlightAppBuilder
/bin/bash ./init.sh

cd ../../dist

export VCPKG_ROOT="${SCRIPT_DIR}/../../docker_working/vcpkg"
/bin/bash ./deps/scripts/install_deps_vcpkg.sh

cd docker_working
cmake -S ../../ -B ./build -G Ninja -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" -DHEADLESS=ON -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXE_LINKER_FLAGS=-latomic -DCPACK_GENERATOR=TGZ
cd build
cmake --build . --target prepare_media
cmake --build . --target package --config Release

#then export out the package