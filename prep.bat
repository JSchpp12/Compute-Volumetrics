SET mypath=%~dp0
SET wpath=%mypath:~0,-1%

IF not exist "%wpath%/prep_work/openVDB" mkdir "%wpath%/prep_work/openVDB"
IF not exist "%wpath%/vsproj" mkdir "%wpath%/vsproj"

cd "%wpath%/prep_work/openVDB"

cmake %wpath%/extern/openVDB -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=<TOOL_PATH> -DCMAKE_INSTALL_PREFIX=%wpath%/libs/openVDB/Debug -DUSE_EXPLICIT_INSTANTIATIONS=OFF -DOPENVDB_CORE_STATIC=OFF -DCMAKE_CXX_STANDARD=20
cmake --build . --parallel 6 --config Debug --target install
cmake %wpath%/extern/openVDB -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=<TOOL_PATH> -DCMAKE_INSTALL_PREFIX=%wpath%/libs/openVDB/Release -DUSE_EXPLICIT_INSTANTIATIONS=OFF -DOPENVDB_CORE_STATIC=OFF -DCMAKE_CXX_STANDARD=20
cmake --build . --parallel 6 --config Release --target install

cd "%wpath%/vsproj"

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=<TOOL_PATH> -DCMAKE_CXX_STANDARD=20 ..