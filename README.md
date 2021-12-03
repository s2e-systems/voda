# Video over DDS Application (VoDA) #

The Video over the Data Distribution Service Application (VoDA) is a software which allows real-time video data from a webcam to be transmitted using GStreamer and the Data Distribution Services (DDS) middleware. Using DDS as a communication mechanism, allows the video data to be transmitted in a data-centric manner with all the functionalities and advantages of the DDS Quality of Service (QoS) specifications. A demonstration video and detailed technical description is available on our website at http://www.s2e-systems.com/our-projects/videooverdds/

## Dependencies ##
The project is developed using only open source and cross platform dependencies. 

- GStreamer development and runtime  (v 1.16.0)
  - https://gstreamer.freedesktop.org
- ADLINK VortexOpenSplice DDS Community Edition (v 6.9.21)
  - https://github.com/ADLINK-IST/opensplice/releases  
- Qt (v 6.2)
  - https://www.qt.io/
- CMake (v 3.1)
  - https://cmake.org
- Optionally: GoogleTest (v 1.8.1)
  - https://github.com/google/googletest.git

While other version of these components might work, these are the ones that should work.

## More information ##
If you would like to use this software, have any questions, or want to know more about S2E Software, Systems and Control you can visit our website at http://www.s2e-systems.com

# Build instructions #
To build the software the CMakeLists.txt files use the following environment variables to search for the dependent libraries:
1. OSPL_HOME pointing to the folder where OpenSplice is installed
2. GSTREAMER_1_0_ROOT_X86_64 to the folder where GStreamer is installed

For run the software the shared library binaries must be added to the PATH environment variable in Windows and to the ldconfig search directories in Linux. In Windows that may be for example:
1. %OSPL_HOME%\bin
2. %GSTREAMER_1_0_ROOT_X86_64%\bin
3. %QTDIR%\bin

