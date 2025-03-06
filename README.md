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

1. Clone this repository making sure to recurse submodules

2. Run the corresponding bat or sh file for your operating system and vcpkg/cmake will handle the rest!

## Troubleshooting

