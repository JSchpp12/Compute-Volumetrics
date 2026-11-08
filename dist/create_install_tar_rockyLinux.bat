@echo off
setlocal

REM Resolve the directory that this script resides in
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "SCRIPT_DIR=%%~fI"

REM The repository root is one level up from dist
for %%I in ("%SCRIPT_DIR%\..") do set "PARENT_DIR=%%~fI"

REM Build variant(s): release (default), debug, or both; compiler: clang
REM (default) or gcc. Examples:
REM   create_install_tar_rockyLinux.bat
REM   create_install_tar_rockyLinux.bat debug --compiler clang
REM   create_install_tar_rockyLinux.bat both --compiler gcc
if "%~1"=="" (
  set "BUILD_ARGS=release"
) else (
  set "BUILD_ARGS=%*"
)

echo Repository root: "%PARENT_DIR%"
echo Build variant(s): %BUILD_ARGS%

REM Build the portability image
docker build -f "%SCRIPT_DIR%\deps\rockyLinux_dockerfile" -t starlight:rockyLinux "%SCRIPT_DIR%\deps" || goto :error

REM Run the packaging build inside the container
docker run --rm ^
  -v "%PARENT_DIR%:/app" ^
  -w /app ^
  starlight:rockyLinux ^
  /bin/bash ./dist/deps/scripts/build_install.sh %BUILD_ARGS% || goto :error

exit /b 0

:error
exit /b 1
