SET mypath=%~dp0
SET wpath=%mypath:~0,-1%

IF not exist "%wpath%/prep_work/tbb" mkdir "%wpath%/prep_work/tbb"
IF not exist "%wpath%/prep_work/blosc" mkdir "%wpath%/prep_work/blosc"
IF not exist "%wpath%/prep_work/zlib" mkdir "%wpath%/prep_work/zlib"
IF not exist "%wpath%/prep_work/openVDB" mkdir "%wpath%/prep_work/openVDB"

CD "%wpath%/prep_work/tbb"

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=%wpath%/libs/tbb -DTBB_TEST=OFF %wpath%/extern/tbb
cmake --build . --config DEBUG
cmake -DCOMPONENT=devel -DBUILD_TYPE=DEBUG -P cmake_install.cmake

cmake --build . --config relwithdebinfo
cmake -DCOMPONENT=devel -DBUILD_TYPE=relwithdebinfo -P cmake_install.cmake

CD "%wpath%/prep_work/blosc"

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=%wpath%/libs/blosc %wpath%/extern/blosc
cmake --build . --config DEBUG --target INSTALL
cmake --build . --config relwithdebinfo --target INSTALL

cd "%wpath%/prep_work/zlib"

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=%wpath%/libs/zlib %wpath%/extern/zlib
cmake --build . --target install

cd "%wpath%/prep_work/openVDB"

cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_INSTALL_PREFIX=%wpath%/libs/openVDB -DTBB_ROOT="%wpath%/libs/tbb" -DBlosc_ROOT=%wpath%/libs/blosc -DZLIB_ROOT=%wpath%/libs/zlib %wpath%/extern/openVDB
cmake --build . --config RELEASE --target install
@REM try relwithdebinfo next 