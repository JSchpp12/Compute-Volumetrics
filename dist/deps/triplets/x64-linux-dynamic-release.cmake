# Release-only variant of the community x64-linux-dynamic triplet used by the
# dist packaging build (rockyLinux_dockerfile).  VCPKG_BUILD_TYPE=release skips
# building the debug variants of every dependency, roughly halving the vcpkg
# share of the packaging build.  Resolved through -DVCPKG_OVERLAY_TRIPLETS.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_FIXUP_ELF_RPATH ON)

set(VCPKG_BUILD_TYPE release)