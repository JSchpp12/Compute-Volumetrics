SET mypath=%~dp0
SET wpath=%mypath:~0,-1%

if not exist "%wpath%/build" mkdir "%wpath%/build"

call "%wpath%/extern/vcpkg/bootstrap-vcpkg.bat"

start "" /wait "%wpath%/extern/vcpkg/vcpkg.exe" "integrate" "install" 

cd "%wpath%/build"

cmake -DCMAKE_TOOLCHAIN_FILE="%wpath%/extern/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release ..
