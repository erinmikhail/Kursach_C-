FROM gcc:13-bookworm

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    gdb \
    make \
    pkg-config \
    clang-16 \
    lldb-16 \
    llvm-16 \
    git \
    curl \
    wget \
    zip \
    unzip \
    tar \
    valgrind \
    heaptrack \
    nlohmann-json3-dev \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y ccache && \
    echo 'export PATH="/usr/lib/ccache:$PATH"' >> ~/.bashrc

WORKDIR /workspace

RUN git config --system --add safe.directory /workspace

ENV DEBIAN_FRONTEND=dialog

RUN g++ --version && cmake --version