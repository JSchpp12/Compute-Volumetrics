@echo off
setlocal

REM Resolve the directory that this script resides in
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "SCRIPT_DIR=%%~fI"

REM The repository root is one level up from dist
for %%I in ("%SCRIPT_DIR%\..") do set "PARENT_DIR=%%~fI"

echo Repository root: "%PARENT_DIR%"

REM Build the portability image
docker build -f "%SCRIPT_DIR%\deps\rockyLinux_dockerfile" -t starlight:rockyLinux "%SCRIPT_DIR%\deps" || goto :error

REM Run the packaging build inside the container
docker run --rm ^
  -v "%PARENT_DIR%:/app" ^
  -w /app ^
  starlight:rockyLinux ^
  /bin/bash ./dist/deps/scripts/build_install.sh || goto :error

exit /b 0

:error
exit /b 1
