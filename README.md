# Volumetric Rendering with Vulkan and OpenVDB

## About

This is a template for applications utilizing Starlight.

## Requirements

The following are required in order to build this project: 

- Python 3.12 or newer
- CMake
- Compiler with c++ 20 support 
- Vulkan SDK Version: 1.4.313.0
- Dedicated GPU (NVIDIA Recommended)

Python Libraries
- pillow 
- rasterio 
- beautifulsoup4

### Additional Linux Setup

The following packages are required from the distribution package manager: 
- pkg-config
- autoconf
- Linux Kernel Headers (linux-headers OR linux-libc-dev)
- bison
- flex
- libtool

## Setup

Clone this repository:

```cmd
git clone --recurse-submodules https://github.com/JSchpp12/Compute-Volumetrics
```

Prepare terrains: 

Navigate to the Terrain Stitcher submodule. Select a terrain from the releases category. Process the terrain following the directions in that project. Place the processed terrain files in the media/terrains directory. 

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

## Development Notes

### Texture Compression and Buffer+Texture Sharing Modes

The target hardware for this renderer is a discrete GPU by NVIDIA. Most buffers and textures utilize VK_SHARING_MODE_CONCURRENT. Additionally, most textures are in a compressed format. On NVIDIA GPUs, the driver ignores the sharing mode. Other vendors, however, do not ignore these flags and the use of them can disable texture compression. This can cause a major performance impact.
