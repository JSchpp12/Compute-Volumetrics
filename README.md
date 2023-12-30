# About
This is a template for applications utilizing Starlight. 

# Requirenments
A major step of the setup for this project is to build the openVDB libs that will be used. The necessary dependencies for openVDB to build are included as submodules. However, there are some caveats. 
The following must be installed: 
- Python3
- All depdencies for [openVDB](https://www.openvdb.org/documentation/doxygen/dependencies.html):
  - zlib
  - blosc
  - tbb
  - boost

# Setup 
1. Clone this repository
```
git clone --recurse-submodules https://github.com/JSchpp12/Compute-Volumetrics.git
```
2. OpenVDB must be built. This project is included as a submodule. However, it itself has depdencies that must be provided. This is the recommended outline of how to setup this library with vcpkg on windows is outlined below: 
    1. Install [vcpkg](https://vcpkg.io/en/) if needed
    2. Install the dependencies for openVDB
    ```cmd
    vcpkg install zlib:x64-windows
    vcpkg install blosc:x64-windows
    vcpkg install tbb:x64-windows
    vcpkg install boost-iostreams:x64-windows
    vcpkg install boost-any:x64-windows
    vcpkg install boost-algorithm:x64-windows
    vcpkg install boost-interprocess:x64-windows
    ```
    3. Prepare soltuions, example provided in prep.bat (make sure to provide DCMAKE_TOOLCHAIN_FILE variable to cmake)
        1. Build openVDB with cmake
        ```cmd
        cd openVDB_build/
        cmake "../extern/openVDB" -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=<TOOLCHAIN_PATH> -DCMAKE_INSTALL_PREFIX=<WHERE_INSTALL> -DUSE_EXPLICIT_INSTANTIATIONS=OFF -DOPENVDB_CORE_STATIC=OFF
        cmake --build . --parallel 6 --config relwithdebinfo --target install
        ```
        2. Create visual studio project for starlight application
        ```cmd
        cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=<TOOLCHAIN_PATH> .
        ```
        3. Open project in visual studio and set the USE_VCPKG setting to YES in the project properties for the starlightapp project. [resource](https://devblogs.microsoft.com/cppblog/vcpkg-is-now-included-with-visual-studio/)
