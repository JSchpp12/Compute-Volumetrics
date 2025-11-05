FROM rockylinux/rockylinux:9-ubi-init

RUN yum update -y && yum clean all \ 
    && dnf install -y 'dnf-command(config-manager)' \ 
    && dnf config-manager --enable crb \ 
    && dnf update -y \ 
    && yum install -y epel-release \ 
    && yum install -y clang clang-tools-extra cmake git git-lfs python3 python3-pip zip unzip autoconf libtool pkg-config wget ninja-build bison flex \ 
    && dnf config-manager --enable appstream \
    && dnf update -y \ 
    && dnf install -y perl \
    && python -m pip install pillow rasterio beautifulsoup4 shapely rtree pyproj
