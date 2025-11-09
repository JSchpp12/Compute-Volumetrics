#!/bin/bash 

/bin/bash ./extern/vcpkg/bootstrap-vcpkg.sh

./extern/vcpkg/vcpkg integrate install

if [ "./extern/StarlightAppBuilder/init.sh"]; then
    echo "StarlightAppBuilder Already Initialized"
else
    git submodule init
    git submodule update --recursive
fi

cd ./extern/StarlightAppBuilder
/bin/bash ./init.sh

cd ../../

mkdir build
mkdir build/Debug

cd build/Debug

cmake -DCMAKE_TOOLCHAIN_FILE="../../extern/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Debug ../../

cmake --build . --config Debug