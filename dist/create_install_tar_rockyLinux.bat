@echo off
setlocal

REM Resolve the directory that this script resides in
set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%.") do set "SCRIPT_DIR=%%~fI"

REM The repository root is one level up from dist
for %%I in ("%SCRIPT_DIR%\..") do set "PARENT_DIR=%%~fI"

set "IMAGE_NAME=starlight:rockyLinux"
set "DOCKERFILE=%SCRIPT_DIR%\deps\rockyLinux_dockerfile"
set "BUILD_CONTEXT=%SCRIPT_DIR%\deps"
REM Label stamped on the image at build time with the Dockerfile's hash.
set "IMAGE_LABEL_KEY=starlight.dockerfile.sha256"

REM Build variant(s): release (default), debug, or both; compiler: clang
REM (default) or gcc. Examples:
REM   create_install_tar_rockyLinux.bat
REM   create_install_tar_rockyLinux.bat debug --compiler clang
REM   create_install_tar_rockyLinux.bat both --compiler gcc
REM   create_install_tar_rockyLinux.bat release --rebuild-image
REM --rebuild-image rebuilds the rockyLinux image even when the existing one
REM was built from the current Dockerfile.
set "REBUILD_IMAGE=0"
set "BUILD_ARGS="

:parse_args
if "%~1"=="" goto args_parsed
if /i "%~1"=="--rebuild-image" (
  set "REBUILD_IMAGE=1"
  shift
  goto parse_args
)
set "BUILD_ARGS=%BUILD_ARGS% %~1"
shift
goto parse_args
:args_parsed

if "%BUILD_ARGS%"=="" set "BUILD_ARGS=release"

echo Repository root: "%PARENT_DIR%"
echo Build variant(s): %BUILD_ARGS%

REM Fingerprint the Dockerfile so a stale image is rebuilt when it changes.
REM The Dockerfile has no COPY/ADD, so the rest of the build context does not
REM affect the resulting image.
set "DOCKERFILE_HASH="
for /f "skip=1 delims=" %%H in ('certutil -hashfile "%DOCKERFILE%" SHA256 2^>nul') do (
  if not defined DOCKERFILE_HASH set "DOCKERFILE_HASH=%%H"
)
REM Older certutil builds print the hash in space-separated byte groups.
if defined DOCKERFILE_HASH set "DOCKERFILE_HASH=%DOCKERFILE_HASH: =%"

REM The image is current when it carries the label matching the file's hash.
REM Without a hash there is no way to tell, so fall back to always building.
set "IMAGE_CURRENT=0"
if defined DOCKERFILE_HASH (
  docker image inspect -f "{{json .Config.Labels}}" "%IMAGE_NAME%" 2>nul | findstr /c:"%DOCKERFILE_HASH%" >nul
  if not errorlevel 1 set "IMAGE_CURRENT=1"
)

if "%REBUILD_IMAGE%"=="1" goto rebuild_image
if "%IMAGE_CURRENT%"=="1" (
  echo Reusing existing image %IMAGE_NAME% ^(matches Dockerfile; pass --rebuild-image to force a rebuild^)
  goto run_build
)

:rebuild_image
if "%REBUILD_IMAGE%"=="1" (
  echo Rebuilding image %IMAGE_NAME% ^(--rebuild-image^)
) else (
  echo Building image %IMAGE_NAME% from %DOCKERFILE%
)
set "LABEL_ARG="
if defined DOCKERFILE_HASH set "LABEL_ARG=--label %IMAGE_LABEL_KEY%=%DOCKERFILE_HASH%"
docker build -f "%DOCKERFILE%" -t "%IMAGE_NAME%" %LABEL_ARG% "%BUILD_CONTEXT%" || goto :error

:run_build
REM Run the packaging build inside the container
docker run --rm ^
  -v "%PARENT_DIR%:/app" ^
  -w /app ^
  "%IMAGE_NAME%" ^
  /bin/bash ./dist/deps/scripts/build_install.sh %BUILD_ARGS% || goto :error

exit /b 0

:error
exit /b 1
