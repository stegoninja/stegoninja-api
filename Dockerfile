# Builder stage
FROM ubuntu:focal AS builder

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install dependencies
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
    git \
    ca-certificates \
    tzdata \
    uuid-dev && \
    rm -rf /var/lib/apt/lists/*

# Build libhttpserver
WORKDIR /tmp
RUN git clone https://github.com/etr/libhttpserver.git && \
    cd libhttpserver && \
    ./bootstrap && \
    mkdir build && \
    cd build && \
    ../configure --prefix=/usr/local && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Compile application
WORKDIR /app
COPY webserver.cpp .
RUN mkdir include
RUN mkdir web
COPY include/ include/
COPY web/ web/
RUN g++ -std=c++17 -Iinclude -o stegoninja webserver.cpp web/* -lhttpserver -lpthread -lssl -lcrypto -lmicrohttpd -lgnutls -luuid

# Final stage
FROM ubuntu:focal

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Install runtime dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    libssl1.1 \
    libmicrohttpd12 \
    libgnutls30 \
    tzdata && \
    rm -rf /var/lib/apt/lists/*

# Copy built artifacts
COPY --from=builder /usr/local/lib/libhttpserver.so* /usr/local/lib/
COPY --from=builder /usr/local/include/httpserver.hpp /usr/local/include/
COPY --from=builder /app/stegoninja /app/stegoninja

# Update shared library cache
RUN ldconfig /usr/local/lib

# Set up application
WORKDIR /app
COPY index.html .
RUN mkdir uploads
RUN mkdir results
RUN mkdir secrets
RUN mkdir extracts

# Expose port and run
EXPOSE 8080
CMD ["./stegoninja"]