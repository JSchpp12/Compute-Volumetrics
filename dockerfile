
FROM rockylinux/rockylinux:9-ubi-init

# Base toolchain and Python libs
RUN yum update -y && yum clean all \
    && dnf install -y 'dnf-command(config-manager)' \
    && dnf config-manager --enable crb \
    && yum install -y epel-release xz \
    && yum install -y clang clang-tools-extra git git-lfs python3 python3-pip zip unzip autoconf autoconf-archive automake libtool pkg-config wget ninja-build bison flex \
    && yum clean all \ 
    && dnf config-manager --enable appstream \
    && dnf install -y perl gcc-toolset-14-libatomic-devel glibc-devel libstdc++-devel \
    && dnf clean all \ 
    && python3 -m pip install --no-cache-dir pillow rasterio beautifulsoup4 shapely rtree pyproj

ARG CMAKE_VERSION=4.2.1
RUN set -eux \
    && wget -O /tmp/cmake-linux-x86_64.sh "https://cmake.org/files/v4.2/cmake-${CMAKE_VERSION}-linux-x86_64.sh" \
    && sh /tmp/cmake-linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm -f /tmp/cmake-linux-x86_64.sh \
    && cmake --version

#vulkan SDK
# Install Vulkan SDK 1.4.313.0 (Linux tarball from LunarG)
ARG VULKAN_SDK_VERSION=1.4.313.0
ENV VULKAN_SDK=/opt/VulkanSDK/${VULKAN_SDK_VERSION}
ENV VULKAN_SDK_ROOT=${VULKAN_SDK}
RUN set -eux \
    && mkdir -p /opt/VulkanSDK \
    && cd /opt/VulkanSDK \
    && wget -O vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz \
         https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/linux/vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz \
    && tar -xf vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz \
    && rm vulkansdk-linux-x86_64-${VULKAN_SDK_VERSION}.tar.xz \
    && chmod +x ${VULKAN_SDK}/setup-env.sh \
    # Ensure the unversioned libvulkan.so exists for CMake's FindVulkan
    && if [ -f "${VULKAN_SDK}/x86_64/lib/libvulkan.so.1" ] && [ ! -f "${VULKAN_SDK}/x86_64/lib/libvulkan.so" ]; then \
         ln -s libvulkan.so.1 ${VULKAN_SDK}/x86_64/lib/libvulkan.so ; \
       fi

#cuda
RUN set -eux \
    && dnf config-manager --add-repo \
         https://developer.download.nvidia.com/compute/cuda/repos/rhel9/x86_64/cuda-rhel9.repo \
    && dnf clean all \
    && dnf -y install cuda-toolkit \
    && echo 'export PATH=/usr/local/cuda/bin:$PATH' >> /etc/profile.d/cuda.sh \
    && echo 'export LD_LIBRARY_PATH=/usr/local/cuda/lib64:${LD_LIBRARY_PATH}' >> /etc/profile.d/cuda.sh