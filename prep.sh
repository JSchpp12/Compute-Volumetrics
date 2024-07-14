#! /bin/bash
VCPKG_PATH="C:\Users\jacob\vcpkg\scripts\buildsystems\vcpkg.cmake"
WPATH=$PWD

if ! test -d prep_work; then
    mkdir prep_work
fi
if ! test -d prep_work/openVDB; then
    mkdir prep_work/openVDB
fi

#move manifest info
cp $WPATH/vcpkg.json $WPATH/extern/openvdb/
cp $WPATH/vcpkg-configuration.json $WPATH/extern/openvdb/

cd prep_work/openVDB

#can actually do the build here...
cmake $WPATH/extern/openvdb -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=$VCPKG_PATH -DCMAKE_INSTALL_PREFIX=$WPATH/libs/openVDB/Debug -DUSE_EXPLICIT_INSTANTIATIONS=OFF -DOPENVDB_CORE_STATIC=OFF -DCMAKE_CXX_STANDARD=20 -DOPENVDB_BUILD_VDB_PRINT=OFF -DBLOSC_USE_EXTERNAL_SOURCES=OFF
cmake --build . --parallel 12 --config debug --target install
cmake $WPATH/extern/openvdb -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_PATH -DCMAKE_INSTALL_PREFIX=$WPATH/libs/openVDB/Release -DUSE_EXPLICIT_INSTANTIATIONS=OFF -DOPENVDB_CORE_STATIC=OFF -DCMAKE_CXX_STANDARD=20 -DOPENVDB_BUILD_VDB_PRINT=OFF -DBLOSC_USE_EXTERNAL_SOURCES=OFF
cmake --build . --parallel 12 --config release --target install