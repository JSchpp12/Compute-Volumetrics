# Volumetric Rendering with Vulkan and OpenVDB

## About

This is a template for applications utilizing Starlight.

## Requirements

The following are required in order to build this project: 

- Python 3.12 or newer
- CMake
- Compiler with c++ 20 support 
- Vulkan SDK Version: 1.4.313

## Setup

Clone this repository:

```cmd
git clone --recurse-submodules https://github.com/JSchpp12/Compute-Volumetrics
```

### Project Setup

#### StarlightAppBuilder

Located in extern/StarlightAppBuilder. Make sure to update submodules:

```cmd
git submodule init
git submodule update
```

Run coresponding bat/shell init script in extern/StarlightAppBuilder.

#### Application Setup

Run the coresponding bat/shell build script located in the root of this project. For windows, this might be build_release.bat. VCPKG/CMake will handle the rest!

## Troubleshooting
