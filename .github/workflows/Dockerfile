FROM ubuntu:20.04 as Builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    git \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /home
RUN git clone --depth 1 --branch 0.10.2 https://github.com/eclipse-cyclonedds/cyclonedds.git
RUN cmake -B cyclonedds-build -S cyclonedds -DCMAKE_INSTALL_PREFIX=cyclonedds-install \
    && cmake --build cyclonedds-build --parallel 5 --target install
RUN git clone --depth 1 --branch 0.10.2 https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git
RUN cmake -B cyclonedds-cxx-build -S cyclonedds-cxx -DCMAKE_INSTALL_PREFIX=cyclonedds-install -DCMAKE_PREFIX_PATH=`pwd`/cyclonedds-install/lib/cmake/CycloneDDS \
    && cmake --build cyclonedds-cxx-build --parallel 5 --target install

FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
COPY --from=Builder /home/cyclonedds-install /usr/local/
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libgstreamer-plugins-base1.0-dev \
    && rm -rf /var/lib/apt/lists/*