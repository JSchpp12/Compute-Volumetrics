VCPKG_PATH="C:\Users\jacob\vcpkg\scripts\buildsystems\vcpkg.cmake"
WPATH=$PWD

if ! test -d vsproj; then
    mkdir vsproj
fi

cd vsproj

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="%vcpkg_path%" -DCMAKE_CXX_STANDARD=20 ..