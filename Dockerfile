# Dockerfile for nsrun development and testing
FROM ubuntu:22.04

# Install required packages
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make \
    iproute2 \
    bridge-utils \
    wget \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /nsrun
COPY . .

RUN make clean && make

RUN mkdir -p rootfs && \
    cd rootfs && \
    wget https://dl-cdn.alpinelinux.org/alpine/v3.18/releases/x86_64/alpine-minirootfs-3.18.4-x86_64.tar.gz && \
    tar xzf alpine-minirootfs-3.18.4-x86_64.tar.gz && \
    rm alpine-minirootfs-3.18.4-x86_64.tar.gz

ENV PATH="/nsrun:${PATH}"

CMD ["/bin/bash"]