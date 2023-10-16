FROM ubuntu:22.04 as CycloneDdsBuilder
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /home
RUN apt-get update && apt-get install -y build-essential cmake git && rm -rf /var/lib/apt/lists/*
RUN git clone --depth 1 --branch 0.10.2 https://github.com/eclipse-cyclonedds/cyclonedds.git \
    && cmake -S cyclonedds -B cyclonedds-build --install-prefix /usr/local/cyclonedds \
    && cmake --build cyclonedds-build --parallel 5 --target install \
    && rm -r cyclonedds-build cyclonedds
RUN git clone --depth 1 --branch 0.10.2 https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git \
    && cmake -S cyclonedds-cxx -B cyclonedds-cxx-build -DCycloneDDS_DIR=/usr/local/cyclonedds/lib/cmake/CycloneDDS --install-prefix /usr/local/cyclonedds \
    && cmake --build cyclonedds-cxx-build --parallel 5 --target install \
    && rm -r cyclonedds-cxx-build cyclonedds-cxx

FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential cmake libgstreamer-plugins-base1.0-dev \
    && rm -rf /var/lib/apt/lists/*
COPY --from=CycloneDdsBuilder /usr/local/cyclonedds /usr/local/
ENV ANDROID_HOME=/android_sdk
WORKDIR /home/root
