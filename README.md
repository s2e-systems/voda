# Video over DDS Application (VoDA) #

The Video over DDS Application (VoDA) is a software which allows real-time video data from a webcam to be transmitted using GStreamer and the Data Distribution Services (DDS) middleware. Using DDS as a communication mechanism, allows the video data to be transmitted in a data-centric manner with all the functionalities and advantages of the DDS Quality of Service (QoS) specifications. A demonstration video and detailed technical description is available on our website at http://www.s2e-systems.com/our-projects/videooverdds/

## Dependencies ##
The project is developed using Qt Creator and this is the recommended IDE to build the software. To be able to build and run the software the following packages must be available on your system:

- GStreamer v 1.12 (https://gstreamer.freedesktop.org/)
- Adlink IST Opensplice Community edition v6.7.1 (http://www.prismtech.com/dds-community/software-downloads)
- Qt 5.9.1 (https://www.qt.io/)

While other version of these components might work, these are the ones used for development.

##System configuration##

To build the software the Qt Creator pri files use the following environment variables to search for the dependent libraries:

1. OSPL_HOME pointing to the folder where Opensplice is installed
2. GSTREAMER_1_0_ROOT_X86_64 to the folder where GStreamer is installed

If you have problems building start by checking that these variables exist and are correctly configured.
The software should also build on Linux and cross-compile for other platforms (e.g Linux on ARM computers), though the build steps are dependent on your own configuration.

##More information##

If you would like to use this software, have any questions, or want to know more about S2E Software, Systems and Control you can visit our website at http://www.s2e-systems.com