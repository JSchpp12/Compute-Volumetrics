#!/bin/bash 

/bin/bash ./extern/vcpkg/bootstrap-vcpkg.sh

./extern/vcpkg/vcpkg integrate install

mkdir build

cd build

cmake -DCMAKE_TOOLCHAIN_FILE="../extern/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Debug ..
