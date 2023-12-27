#entrypoint for setup 

#builds openVDB for use with application

import os
import subprocess

if __name__ == "__main__":
    cmake_compiler = None
    cmake_command = None

    dirExtern = os.path.join(os.getcwd(), "extern")
    workingDir = os.path.join(os.getcwd(), "prep_work")
    resLibDir = os.path.join(os.getcwd(), "libs")

    openVDB_deps = [
        os.path.join(dirExtern, "tbb"),
        os.path.join(dirExtern, "blosc"),
        os.path.join(dirExtern, "zlib")
    ]

    if (not os.path.isdir(workingDir)):
        os.mkdir(workingDir)
    if (not os.path.isdir(resLibDir)):
        os.mkdir(resLibDir)

    root_blosc = (None, None)
    root_tbb = (None, None)
    root_zlib = (None, None)

    for libRoot in openVDB_deps:
        libName = os.path.basename(libRoot)
        libInstallDir = os.path.join(resLibDir, libName)
        if not os.path.isdir(libInstallDir):
            os.mkdir(libInstallDir)
        libInstallDirDebug = os.path.join(libInstallDir, "Debug")
        if not os.path.isdir(libInstallDirDebug):
            os.mkdir(libInstallDirDebug)
        libInstallDirRelease = os.path.join(libInstallDir, "Release")
        if not os.path.isdir(libInstallDirRelease):
            os.mkdir(libInstallDirRelease)

        libWorkingDir = os.path.join(workingDir, libName)
        if not os.path.isdir(libWorkingDir):
            os.mkdir(libWorkingDir)

        libWorkingDirDebug = os.path.join(libWorkingDir, "Debug")
        if not os.path.isdir(libWorkingDirDebug):
            os.mkdir(libWorkingDirDebug)

        libWorkingDirRelease = os.path.join(libWorkingDir, "Release")
        if not os.path.isdir(libWorkingDirRelease):
            os.mkdir(libWorkingDirRelease)
        
        cmake_call_path = os.path.relpath(libRoot, libWorkingDir)
        pathToSourceDebug = os.path.relpath(libRoot, libWorkingDirDebug)
        pathToSourceRelease = os.path.relpath(libRoot, libWorkingDirRelease)
        cmake_install_debug_path = os.path.relpath(libInstallDirDebug, libWorkingDir)
        cmake_install_release_path = os.path.relpath(libInstallDirRelease, libWorkingDir)
        cmake_conf_debug_cmd = ["cmake", "-G", "Visual Studio 17 2022", "-A", "x64", "-DCMAKE_BUILD_TYPE=Debug", f"-DCMAKE_INSTALL_PREFIX={cmake_install_debug_path}", cmake_call_path]
        cmake_conf_release_cmd = ["cmake", "-G", "Visual Studio 17 2022", "-A", "x64", "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_INSTALL_PREFIX={cmake_install_release_path}", cmake_call_path]
        cmake_build_cmd_release = ["cmake", "--build", ".", "--config", "Release", "--target", "Install"]
        cmake_build_cmd_debug = ["cmake", "--build", ".", "--config", "Debug", "--target", "Install"]

        subprocess.run(cmake_conf_debug_cmd, cwd=libWorkingDir)
        subprocess.run(cmake_build_cmd_debug, cwd=libWorkingDir)
        subprocess.run(cmake_conf_release_cmd, cwd=libWorkingDir)
        subprocess.run(cmake_build_cmd_release, cwd=libWorkingDir)

        root_info = (os.path.abspath(os.path.join(libWorkingDir, cmake_install_debug_path)), os.path.abspath(os.path.join(libWorkingDir, cmake_install_release_path)))
        if "boost" in libName: 
            root_boost = root_info
        elif "tbb" in libName:
            root_tbb = root_info
        elif "blosc" in libName:
            root_blosc = root_info
        elif "zlib" in libName:
            root_zlib = root_info

    openVDB_path = os.path.join(dirExtern, "openvdb")
    openVDB_working_path = os.path.join(workingDir, "openVDB")
    if not os.path.isdir(openVDB_working_path):
        os.mkdir(openVDB_working_path)

    openVDB_install_path_release = os.path.join(resLibDir, "openVDB", "Release")
    openVDB_install_path_debug = os.path.join(resLibDir, "openVDB", "Debug")
    cmake_openVDB_path = os.path.relpath(openVDB_path, openVDB_working_path)
    cmake_config_openVDB_cmd_debug = ["cmake", "-G", "Visual Studio 17 2022", "-A", "x64", cmake_openVDB_path, "-DCMAKE_BUILD_TYPE=Debug", f"-DCMAKE_INSTALL_PREFIX={openVDB_install_path_debug}", 
                                        "-D", f"TBB_ROOT={root_tbb[0]}",
                                        "-D", f"ZLIB_ROOT={root_zlib[0]}",
                                        "-D", f"Blosc_ROOT={root_blosc[0]}"]
    cmake_config_openVDB_cmd_release = ["cmake", "-G", "Visual Studio 17 2022", "-A", "x64", cmake_openVDB_path, "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_INSTALL_PREFIX={openVDB_install_path_release}", 
                                        "-D", f"TBB_ROOT={root_tbb[1]}",
                                        "-D", f"ZLIB_ROOT={root_zlib[1]}",
                                        "-D", f"Blosc_ROOT={root_blosc[1]}"]
    cmake_build_cmd_release = ["cmake", "--build", ".", "--config", "Release", "--target", "Install"]
    cmake_build_cmd_debug = ["cmake", "--build", ".", "--config", "Debug", "--target", "Install"]

    # subprocess.run(cmake_config_openVDB_cmd_debug, cwd=openVDB_working_path)
    # subprocess.run(cmake_build_cmd_debug, cwd=openVDB_working_path)
    subprocess.run(cmake_config_openVDB_cmd_release, cwd=openVDB_working_path)
    subprocess.run(cmake_build_cmd_release, cwd=openVDB_working_path)