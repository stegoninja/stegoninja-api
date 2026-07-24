# StegoNinja web server.
#
# Single-stage image built on Ubuntu 22.04. Rationale:
#   * libhttpserver's current release requires a C++20 compiler; Ubuntu 22.04
#     ships g++ 11 which provides it (Ubuntu 20.04 / g++ 9 does not).
#   * The video LSB endpoint needs OpenCV (with its FFmpeg videoio backend);
#     keeping build + runtime in one stage avoids fragile shared-library copies.
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Build + runtime dependencies.
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    autoconf \
    automake \
    libtool \
    pkg-config \
    libssl-dev \
    libgnutls28-dev \
    libmicrohttpd-dev \
    libopencv-dev \
    git \
    ca-certificates \
    tzdata \
    uuid-dev && \
    rm -rf /var/lib/apt/lists/*

# Build libhttpserver from source (not packaged in Ubuntu).
# Pinned to 0.19.0: the master branch now requires libmicrohttpd >= 1.0.0, which
# is newer than what Ubuntu 22.04 packages (0.9.75). 0.19.0 builds against the
# packaged libmicrohttpd and still provides the get_arg_flat/get_content API.
WORKDIR /tmp
RUN git clone --branch 0.19.0 --depth 1 https://github.com/etr/libhttpserver.git && \
    cd libhttpserver && \
    ./bootstrap && \
    mkdir build && \
    cd build && \
    ../configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Compile the application.
# Headers live in include/ and are referenced as "include/..." (from
# webserver.cpp) and "../include/..." (from web/*.cpp).
WORKDIR /app
COPY webserver.cpp .
COPY include/ include/
COPY web/ web/
RUN g++ -std=c++20 -Iinclude -o stegoninja webserver.cpp web/*.cpp \
    $(pkg-config --cflags opencv4) \
    -lhttpserver -lpthread -lssl -lcrypto -lmicrohttpd -lgnutls -luuid \
    $(pkg-config --libs opencv4)

# Application setup.
RUN mkdir -p uploads results secrets extracts

# Expose port and run.
EXPOSE 8080
CMD ["./stegoninja"]
