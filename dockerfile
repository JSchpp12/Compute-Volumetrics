FROM ubuntu:22.04

RUN apt-get update && apt-get upgrade -y 
RUN apt-get install -y build-essential gcc git wget curl zip unzip tar make

WORKDIR /docker_build_deps/cmake
# RUN wget https://github.com/Kitware/CMake/releases/download/v3.30.2/cmake-3.30.2-linux-x86_64.sh \ 
#     && chmod +x cmake-3.30.2-linux-x86_64.sh \ 
#     && mkdir /opt/cmake-3.30.2 \ 
#     && ./cmake-3.30.2-linux-x86_64.sh --skip-license --prefix=/opt/cmake-3.30.2 \ 
#     && ln -s /opt/cmake-3.20.2/bin/* /usr/local/bin
RUN test -f /usr/share/doc/kitware-archive-keyring/copyright || wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null \
    && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
    && apt-get update \ 
    && apt-get install -y cmake

WORKDIR /docker_build_deps/vulkan
RUN wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | tee /etc/apt/trusted.gpg.d/lunarg.asc \
    && wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list \ 
    && apt-get update -y \ 
    && apt-get install -y vulkan-sdk
