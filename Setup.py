#entrypoint for setup 

#builds openVDB for use

import os
import subprocess
import platform

if __name__ == "__main__":
    cmake_compiler = None
    cmake_command = None

    if platform.system() == "Windows":
        cmake_command = "cmake.exe"
    else:
        cmake_command = "cmake"
    
    print()
    if platform.release() == "10":
        cmake_compiler = "Visual Studio 11 Win64"

    if cmake_compiler is None: 
        print("This is not a supported platform for automated builds of depdencies.")
        exit()
    
    # cmake_build_dir = os.path.join(os.getcwd(), "libs")

    # cmake_command = [cmake_command, "-G", cmake_compiler, cmake_build_dir]

    work = os.path.join(os.getcwd(), "extern", "tbb")

    subprocess.run(cmake_command)