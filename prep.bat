SET mypath=%~dp0
SET wpath=%mypath:~0,-1%

IF not exist "%wpath%/prep_work/openVDB" mkdir "%wpath%/prep_work/openVDB"
IF not exist "%wpath%/vsproj" mkdir "%wpath%/vsproj"

cd "%wpath%/prep_work/openVDB"

cmake %wpath%/extern/openVDB -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=<TOOL> -DCMAKE_INSTALL_PREFIX=%wpath%/libs/openVDB -DUSE_EXPLICIT_INSTANTIATIONS=OFF -DOPENVDB_CORE_STATIC=OFF
cmake --build . --parallel 6 --config relwithdebinfo --target install

cd "%wpath%/vsproj"

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=<TOOL> ..