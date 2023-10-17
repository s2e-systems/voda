# Video over DDS Application (VoDA) #

The Video over the Data Distribution Service Application (VoDA) is a software which allows real-time video data from a webcam to be transmitted using GStreamer and the Data Distribution Services (DDS) middleware. Using DDS as a communication mechanism, allows the video data to be transmitted in a data-centric manner with all the functionalities and advantages of the DDS Quality of Service (QoS) specifications. A demonstration video and detailed technical description is available on our website at <http://www.s2e-systems.com/our-projects/videooverdds/>
This software should build with CMake on Windows and Linux. The source does also built on ARM computers such as Rasberry Pi's but requires some more build steps.
Additionally a demonstration for Android is available under the android directory. A publisher app and a subscriber app are provided. With that it is possible to stream video from an Android phone to for example a Windows PC and vice versa.

## Dependencies ##

The project is developed using only open source and cross platform dependencies.

- GStreamer development and runtime  (v 1.20.1)
  - <https://gstreamer.freedesktop.org/download>
- Eclipse Cyclone DDS™ (v 0.10.2)
  - <https://github.com/eclipse-cyclonedds/cyclonedds>
  - <https://github.com/eclipse-cyclonedds/cyclonedds-cxx>

Other version of these components probably work as well.

## More information ##

If you would like to use this software, have any questions, or want to know more about S2E Software Systems B.V. you can visit our website at <http://www.s2e-systems.com>

## Build instructions ##

To build the software the CMakeLists.txt files are using the following environment variable to search for the dependent libraries:

- GSTREAMER_1_0_ROOT_X86_64 to the folder where GStreamer is installed

For run the software the shared library binaries must be added to the PATH environment variable in Windows and to the ldconfig search directories in Linux. In Windows that may be for example:

- %GSTREAMER_1_0_ROOT_X86_64%\bin
