SETLOCAL

REM Get script path
SET mypath=%~dp0
SET wpath=%mypath:~0,-1%
SET appBuilderDir=%wpath%\extern\StarlightAppBuilder

REM Ensure build directory exists
if not exist "%wpath%\build\Release" mkdir "%wpath%\build\Release"

REM Bootstrap vcpkg
call "%wpath%\extern\vcpkg\bootstrap-vcpkg.bat"

REM Integrate vcpkg
"%wpath%\extern\vcpkg\vcpkg.exe" integrate install

REM Go to build directory
cd /d "%wpath%\build"

REM Check if AppBuilder is initialized
if exist "%appBuilderDir%" (
    echo StarlightAppBuilder Already Initialized
) else (
    echo Initializing StarlightAppBuilder Project && git submodule init && git submodule update --recursive && cd /d "%appBuilderDir%" && git submodule init && git submodule update && call "%appBuilderDir%\init.bat"
)

REM Configure and build
cd /d "%wpath%\build\Release"
cmake -DCMAKE_TOOLCHAIN_FILE="%wpath%\extern\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release ../../
cmake --build . -j6

ENDLOCAL
