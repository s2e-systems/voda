FROM ubuntu:22.04 as Builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y xz-utils curl && rm -rf /var/lib/apt/lists/*
WORKDIR /home/gstreamer-1.0-android-universal
RUN curl https://gstreamer.freedesktop.org/pkg/android/1.20.5/gstreamer-1.0-android-universal-1.20.5.tar.xz | tar -x
#ADD https://gstreamer.freedesktop.org/pkg/android/1.20.5/gstreamer-1.0-android-universal-1.20.5.tar.xz .
#RUN mkdir gstreamer-1.0-android-universal && tar -xf gstreamer-1.0-android-universal-1.20.5.tar.xz --directory gstreamer-1.0-android-universal && rm gstreamer-1.0-android-universal-1.20.5.tar.xz
WORKDIR /home
# RUN apt-get update && apt-get install -y build-essential cmake git google-android-ndk-installer

# RUN git clone --depth 1 --branch 0.10.2 https://github.com/eclipse-cyclonedds/cyclonedds.git
# RUN git clone --depth 1 --branch 0.10.2 https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git


# RUN cmake -S cyclonedds -B cyclonedds-build --install-prefix `pwd`/cyclonedds-install
# RUN cmake --build cyclonedds-build --parallel 5 --target install

# RUN cmake -S cyclonedds-cxx -B cyclonedds-cxx-build --install-prefix `pwd`/cyclonedds-install -DCMAKE_PREFIX_PATH=`pwd`/cyclonedds-install/lib/cmake/CycloneDDS
# RUN cmake --build cyclonedds-cxx-build --parallel 5 --target install


# RUN cmake -S cyclonedds -B cyclonedds-build-android/x86_64 -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=x86_64 -DENABLE_SECURITY=Off -DBUILD_IDLC=On -DBUILD_SHARED_LIBS=Off \
#     --toolchain /usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake --install-prefix `pwd`/cyclonedds-install/android/x86_64
# RUN cmake --build cyclonedds-build-android/x86_64 --parallel 5 --target install
# RUN cmake -S cyclonedds-cxx -B cyclonedds-cxx-build-android/x86_64 -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=x86_64 -DBUILD_IDLLIB=On -DBUILD_SHARED_LIBS=Off -DCycloneDDS_DIR=`pwd`/cyclonedds-install/android/x86_64/lib/cmake/CycloneDDS \
#     --toolchain /usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake --install-prefix `pwd`/cyclonedds-install/android/x86_64
# RUN cmake --build cyclonedds-cxx-build-android/x86_64 --parallel 5 --target install

# RUN cmake -S cyclonedds -B cyclonedds-build-android/x86 -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=x86 -DENABLE_SECURITY=Off -DBUILD_IDLC=On -DBUILD_SHARED_LIBS=Off \
#     --toolchain /usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake --install-prefix `pwd`/cyclonedds-install/android/x86
# RUN cmake --build cyclonedds-build-android/x86 --parallel 5 --target install
# RUN cmake -S cyclonedds-cxx -B cyclonedds-cxx-build-android/x86 -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=x86 -DBUILD_IDLLIB=On -DBUILD_SHARED_LIBS=Off -DCycloneDDS_DIR=`pwd`/cyclonedds-install/android/x86/lib/cmake/CycloneDDS \
#     --toolchain /usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake --install-prefix `pwd`/cyclonedds-install/android/x86
# RUN cmake --build cyclonedds-cxx-build-android/x86 --parallel 5 --target install

# RUN cmake -S cyclonedds -B cyclonedds-build-android/arm64-v8a -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=arm64-v8a -DENABLE_SECURITY=Off -DBUILD_IDLC=On -DBUILD_SHARED_LIBS=Off \
#     --toolchain /usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake --install-prefix `pwd`/cyclonedds-install/android/arm64-v8a
# RUN cmake --build cyclonedds-build-android/arm64-v8a --parallel 5 --target install
# RUN cmake -S cyclonedds-cxx -B cyclonedds-cxx-build-android/arm64-v8a -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=arm64-v8a -DBUILD_IDLLIB=On -DBUILD_SHARED_LIBS=Off -DCycloneDDS_DIR=`pwd`/cyclonedds-install/android/arm64-v8a/lib/cmake/CycloneDDS \
#     --toolchain /usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake --install-prefix `pwd`/cyclonedds-install/android/arm64-v8a
# RUN cmake --build cyclonedds-cxx-build-android/arm64-v8a --parallel 5 --target install

# RUN cmake -S cyclonedds -B cyclonedds-build-android/armeabi-v7a -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=armeabi-v7a -DENABLE_SECURITY=Off -DBUILD_IDLC=On -DBUILD_SHARED_LIBS=Off \
#     --toolchain /usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake --install-prefix `pwd`/cyclonedds-install/android/armeabi-v7a
# RUN cmake --build cyclonedds-build-android/armeabi-v7a --parallel 5 --target install
# RUN cmake -S cyclonedds-cxx -B cyclonedds-cxx-build-android/armeabi-v7a -DANDROID_NATIVE_API_LEVEL=28 -DANDROID_ABI=armeabi-v7a -DBUILD_IDLLIB=On -DBUILD_SHARED_LIBS=Off -DCycloneDDS_DIR=`pwd`/cyclonedds-install/android/armeabi-v7a/lib/cmake/CycloneDDS \
#     --toolchain /usr/lib/android-sdk/ndk-bundle/build/cmake/android.toolchain.cmake --install-prefix `pwd`/cyclonedds-install/android/armeabi-v7a
# RUN cmake --build cyclonedds-cxx-build-android/armeabi-v7a --parallel 5 --target install


# FROM ubuntu:22.04
# ENV DEBIAN_FRONTEND=noninteractive
# WORKDIR /home/root
# COPY --from=Builder /home/gstreamer-1.0-android-universal /usr/local/gstreamer-1.0-android-universal
# COPY --from=Builder /home/cyclonedds-install /usr/local/

# RUN apt-get update && apt-get install -y \
#     build-essential \
#     cmake \
#     libgstreamer-plugins-base1.0-dev \
#     ninja-build \
#     android-sdk google-android-ndk-installer sdkmanager\
#     && rm -rf /var/lib/apt/lists/*
# ENV ANDROID_HOME=/lib/android-sdk
# ENV CycloneDDS_ROOT_ANDROID=/usr/local/android
# ENV GSTREAMER_1_0_ROOT_ANDROID=/usr/local/gstreamer-1.0-android-universal
# RUN yes | sdkmanager --licenses

