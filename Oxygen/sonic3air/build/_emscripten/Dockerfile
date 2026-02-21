FROM --platform=$BUILDPLATFORM emscripten/emsdk:4.0.23
RUN apt-get update && \
    apt-get -y install \
    libgl-dev \
    libogg-dev \
    libsdl2-dev \
    python3 \
    python3-pip && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
