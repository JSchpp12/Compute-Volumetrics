# Volumetric Rendering with Vulkan and OpenVDB

This is a template for applications utilizing Starlight.

## Requirenments

A major step of the setup for this project is to build the openVDB libs that will be used. The necessary dependencies for openVDB to build are included as submodules. However, there are some caveats.
The following must be installed:

- Python3
- Cmake
- All depdencies for [openVDB](https://www.openvdb.org/documentation/doxygen/dependencies.html):
  - zlib
  - blosc
  - tbb
  - boost

## Setup

1. Clone this repository

```cmd
git clone --recurse-submodules https://github.com/JSchpp12/Compute-Volumetrics.git
```

2. OpenVDB must be built. This project is included as a submodule. However, it itself has depdencies that must be provided. The recommended outline of how to setup this library with vcpkg on windows is outlined below:
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

    NOTE: On some systems there are issues with boost being found by cmake when using vcpkg. To ensure everything works, install boost as described on the project website.

    3. Prepare solutions, example provided in prep.bat (make sure to provide DCMAKE_TOOLCHAIN_FILE variable to cmake)
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
    4. Copy openVDB.dll to the executable directory. Make sure that the proper build type .dll is placed in .exe directory (i.e. If the .exe is built in debug mode, place the openVDB.dll debug build in the same directory as the exe). Future work to automate this process in a way which will not interfere with vcpkg. If using the prep.bat, the debug and release builds of openVDB will be placed in seperate directories.

## Troubleshooting

1. Required .dll not found while using vcpkg.

If using vcpkg and during runtime, an error is displayed regarding a .dll not being found, this is due to a copy failure or visual studio not properly using vcpkg. In project properties, ensure Use Vcpkg is set to ON. It might also help to set Use Manifest to ON as well.

2. Undefined references to lz4 while compiling openvdb.

This is an issue which occurred while compiling on linux. A possible solution can be to install the lz4 binary for the distro. 