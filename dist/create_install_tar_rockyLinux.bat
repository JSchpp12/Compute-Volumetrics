
@echo off
setlocal enabledelayedexpansion

REM Resolve the directory that this script resides in
set "SCRIPT_DIR=%~dp0"

REM Normalize to a fully-qualified path
for %%I in ("%SCRIPT_DIR%.") do set "SCRIPT_DIR=%%~fI"

REM Go TWO directories up from script location
for %%I in ("%SCRIPT_DIR%..\..") do set "PARENT_DIR=%%~fI"

echo Parent directory: "%PARENT_DIR%"

REM Build Docker image
pushd deps
@REM docker build -f rockyLinux_dockerfile -t starlight:rockyLinux .
popd

REM Run container
docker run -it ^
  -v "%PARENT_DIR%:/app" ^
  -w /app ^
  starlight:rockyLinux ^
  /bin/bash -lc "./dist/deps/scripts/build_install.sh"

endlocal
