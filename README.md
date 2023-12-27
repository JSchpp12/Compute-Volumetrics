# About
This is a template for applications utilizing Starlight. 

# Requirenments
A major step of the setup for this project is to build the openVDB libs that will be used. The necessary dependencies for openVDB to build are included as submodules. However, there are some caveats. 
The following must be installed: 
- Boost (1.80)
- Python3

# Setup 
1. Clone this repository
```
git clone --recurse-submodules https://github.com/JSchpp12/Compute-Volumetrics.git
```
2. Build Depdencies by running the included python setup file. NOTE: This will take some time depending on system.
```
python3 Setup.py
```

