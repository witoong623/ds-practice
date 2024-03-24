FROM nvcr.io/nvidia/deepstream:6.4-gc-triton-devel as devel

ENV TZ=Asia/Bangkok \
    DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y build-essential tmux sudo vim cmake \
    # lines below are necessary for installing glib
    pkg-config \
    python3-dev \
    meson \
    ninja-build \
    libxslt1-dev \
    libpcre2-dev

RUN curl -LO https://download.gnome.org/sources/glib/2.76/glib-2.76.6.tar.xz && \
    tar -xvf glib-2.76.6.tar.xz && \
    cd glib-2.76.6 && \
    meson _build --prefix=/usr --buildtype=release && \
    ninja -C _build && \
    ninja -C _build install && \
    cd .. && rm -rf glib-2.76.6 glib-2.76.6.tar.xz

# specific to DS image 6.4, modify existing user for user ID 1000
RUN usermod -d /home/triton-server -s /bin/bash -aG sudo triton-server \
    && mkhomedir_helper triton-server \
    && echo triton-server ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/triton-server \
    && chmod 0440 /etc/sudoers.d/triton-server

USER triton-server

WORKDIR /home/triton-server
