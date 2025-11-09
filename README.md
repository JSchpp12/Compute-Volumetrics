# Volumetric Rendering with Vulkan and OpenVDB

This rendering application is the core of an ongoing research project which requires the creation of a rigerous dataset of terrain images with varying visibility distances. These images will then be used to train machine learning models to classify atmospheric visibility distance.


<p float="left">
  <img src="./assets/images/nano_volumetric_high.png" width="400" />
  <img src="./assets/images/nano_SDF.png" width="400" /> 
</p>

## Research Goals

### Abstract

Weather significantly impacts the safe operation of aircraft, with fog being a notable concern for safety. A critical measurement that the Federal Aviation Administration (FAA) uses to classify flight-ready conditions is atmospheric visibility. Current technologies for visibility measurements are expensive. The FAA has identified the use of Machine Learning as a primary target in their FY24-28 National Research Plan, and Machine learning models, coupled with low-cost equipment, might provide a viable replacement for traditional estimators. However, public datasets of real world images containing accurate visibility estimations are sparse. Many works have turned to data augmentation or Synthetic Data Engineering as a solution, but synthetic data, when training a model for deployment on real world data, introduces domain shift which negatively impacts model performance. This research work comes in two phases. The first phase proposes to create a tool capable of generating synthetic foggy images of varying visual quality, from simple to photorealistic. Training models on varying quality of data and verifying on the highest quality of synthetic data, will show how visual quality relates to model performance. The second phase proposes to measure model performance when a mix of high quality synthetic data and real world data are used in training. This will demonstrate the ability of synthetic data to minimize the problem of domain shift.

### Overview

The primary goal of this rendering application is in the creation of synthetic images of varying quality. From relatively simple rendering approachs (flat terrains and simple homoenous fog), to a more photorealistic approach (light transport simulation and real world elevation data). As such, there are several external data sources which are required. 

#### Volumes

![](./assets/images/march_bounding_box.png)

### [Early Demo](https://youtu.be/qZRzl9HPQ6Y)


## Development Requirements

The following are required in order to build this project: 

- Python 3.12 or newer
- CMake
- Compiler with c++ 20 support 
- Vulkan SDK Version: 1.4.313.0
- Dedicated GPU (NVIDIA Recommended)
- CUDA (might be removed in future)

Python Libraries
- pillow 
- rasterio 
- beautifulsoup4

```cmd
python -m pip install pillow rasterio beautifulsoup4
```

### Additional Linux Setup

The following packages are required from the distribution package manager: 
- pkg-config
- autoconf
- Linux Kernel Headers (linux-headers OR linux-libc-dev)
- bison
- flex
- libtool

The following are required for display:
- libwayland

## Setup

Clone this repository:

```cmd
git clone --recurse-submodules https://github.com/JSchpp12/Compute-Volumetrics
```

Prepare terrains: 

Navigate to the Terrain Stitcher submodule. Select and download a terrain from the releases category. These can be quite large and is suggested you use something like GitHub CLI. Process the terrain following the directions in that project. Place the processed terrain files in the media/terrains directory. 

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
