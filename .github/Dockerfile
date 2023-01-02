FROM cimg/base:2022.12
RUN git clone --depth=1 https://github.com/eclipse-cyclonedds/cyclonedds.git
RUN git clone --depth=1 https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git
RUN cmake -B cyclonedds-build -S cyclonedds \
    && cmake --build cyclonedds-build --parallel 5 \
    && sudo cmake --install cyclonedds-build
RUN cmake -B cyclonedds-cxx-build -S cyclonedds-cxx \
    && cmake --build cyclonedds-cxx-build --parallel 5 \
    && sudo cmake --install cyclonedds-cxx-build
RUN rm -rf cyclonedds \
    && rm -rf cyclonedds-cxx \
    && rm -rf cyclonedds-build \
    && rm -rf cyclonedds-cxx-build 
RUN sudo apt-get update -y \
    && sudo apt-get install libgstreamer-plugins-base1.0-dev -y
