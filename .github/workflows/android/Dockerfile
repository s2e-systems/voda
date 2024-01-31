FROM ubuntu:22.04 as CycloneDdsAndroidBuilder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y build-essential cmake git wget unzip openjdk-19-jre-headless && rm -rf /var/lib/apt/lists/*
WORKDIR /android_sdk/cmdline-tools/latest
RUN wget https://dl.google.com/android/repository/commandlinetools-linux-10406996_latest.zip \
    && unzip commandlinetools-linux-10406996_latest.zip \
    && mv cmdline-tools/* . \
    && rm -r commandlinetools-linux-10406996_latest.zip cmdline-tools
RUN yes | ./bin/sdkmanager --licenses
RUN ./bin/sdkmanager --install "build-tools;34.0.0" "ndk;25.1.8937393"
ENV ANDROID_HOME=/android_sdk

WORKDIR /home

RUN git clone --depth 1 --branch 0.10.2 https://github.com/eclipse-cyclonedds/cyclonedds.git \
    && cmake -S cyclonedds -B cyclonedds-build --install-prefix /usr/local/cyclonedds \
    && cmake --build cyclonedds-build --parallel 5 --target install \
    && cmake -S cyclonedds -B cyclonedds-build-android/x86_64 -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=x86_64 -DENABLE_SECURITY=Off -DBUILD_IDLC=On -DBUILD_SHARED_LIBS=Off \
    --toolchain /android_sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake --install-prefix /usr/local/cyclonedds/android/x86_64 \
    && cmake --build cyclonedds-build-android/x86_64 --parallel 5 --target install \
    && cmake -S cyclonedds -B cyclonedds-build-android/x86 -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=x86 -DENABLE_SECURITY=Off -DBUILD_IDLC=On -DBUILD_SHARED_LIBS=Off \
    --toolchain /android_sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake --install-prefix /usr/local/cyclonedds/android/x86 \
    && cmake --build cyclonedds-build-android/x86 --parallel 5 --target install \
    && cmake -S cyclonedds -B cyclonedds-build-android/arm64-v8a -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=arm64-v8a -DENABLE_SECURITY=Off -DBUILD_IDLC=On -DBUILD_SHARED_LIBS=Off \
    --toolchain /android_sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake --install-prefix /usr/local/cyclonedds/android/arm64-v8a \
    && cmake --build cyclonedds-build-android/arm64-v8a --parallel 5 --target install \
    && cmake -S cyclonedds -B cyclonedds-build-android/armeabi-v7a -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=armeabi-v7a -DENABLE_SECURITY=Off -DBUILD_IDLC=On -DBUILD_SHARED_LIBS=Off \
    --toolchain /android_sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake --install-prefix /usr/local/cyclonedds/android/armeabi-v7a \
    && cmake --build cyclonedds-build-android/armeabi-v7a --parallel 5 --target install \
    && rm -r cyclonedds-build-android cyclonedds

RUN git clone --depth 1 --branch 0.10.2 https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git \
    && cmake -S cyclonedds-cxx -B cyclonedds-cxx-build -DCycloneDDS_DIR=/usr/local/cyclonedds/lib/cmake/CycloneDDS --install-prefix /usr/local/cyclonedds \
    && cmake --build cyclonedds-cxx-build --parallel 5 --target install \
    && cmake -S cyclonedds-cxx -B cyclonedds-cxx-build-android/x86_64 -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=x86_64 -DBUILD_IDLLIB=On -DBUILD_SHARED_LIBS=Off -DCycloneDDS_DIR=/usr/local/cyclonedds/android/x86_64/lib/cmake/CycloneDDS \
    --toolchain /android_sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake --install-prefix /usr/local/cyclonedds/android/x86_64 \
    && cmake --build cyclonedds-cxx-build-android/x86_64 --parallel 5 --target install \
    && cmake -S cyclonedds-cxx -B cyclonedds-cxx-build-android/x86 -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=x86 -DBUILD_IDLLIB=On -DBUILD_SHARED_LIBS=Off -DCycloneDDS_DIR=/usr/local/cyclonedds/android/x86/lib/cmake/CycloneDDS \
    --toolchain /android_sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake --install-prefix /usr/local/cyclonedds/android/x86 \
    && cmake --build cyclonedds-cxx-build-android/x86 --parallel 5 --target install \
    && cmake -S cyclonedds-cxx -B cyclonedds-cxx-build-android/arm64-v8a -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=arm64-v8a -DBUILD_IDLLIB=On -DBUILD_SHARED_LIBS=Off -DCycloneDDS_DIR=/usr/local/cyclonedds/android/arm64-v8a/lib/cmake/CycloneDDS \
    --toolchain /android_sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake --install-prefix /usr/local/cyclonedds/android/arm64-v8a \
    && cmake --build cyclonedds-cxx-build-android/arm64-v8a --parallel 5 --target install \
    && cmake -S cyclonedds-cxx -B cyclonedds-cxx-build-android/armeabi-v7a -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=armeabi-v7a -DBUILD_IDLLIB=On -DBUILD_SHARED_LIBS=Off -DCycloneDDS_DIR=/usr/local/cyclonedds/android/armeabi-v7a/lib/cmake/CycloneDDS \
    --toolchain /android_sdk/ndk/25.1.8937393/build/cmake/android.toolchain.cmake --install-prefix /usr/local/cyclonedds/android/armeabi-v7a \
    && cmake --build cyclonedds-cxx-build-android/armeabi-v7a --parallel 5 --target install \
    && rm -r cyclonedds-cxx-build-android cyclonedds-cxx


FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    cmake ninja-build openjdk-19-jre-headless xz-utils curl \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /usr/local/gstreamer-1.0-android-universal
RUN curl https://gstreamer.freedesktop.org/pkg/android/1.22.6/gstreamer-1.0-android-universal-1.22.6.tar.xz | tar --extract --xz
WORKDIR /home/root
COPY --from=CycloneDdsAndroidBuilder /android_sdk /android_sdk
COPY --from=CycloneDdsAndroidBuilder /usr/local/cyclonedds /usr/local/
ENV ANDROID_HOME=/android_sdk
ENV CycloneDDS_ROOT_ANDROID=/usr/local/android
ENV GSTREAMER_1_0_ROOT_ANDROID=/usr/local/gstreamer-1.0-android-universal

