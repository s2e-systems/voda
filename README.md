# Video over DDS Application (VoDA) #

The Video over the Data Distribution Service Application (VoDA) is a software which allows real-time video data from a webcam to be transmitted using GStreamer and the Data Distribution Services (DDS) middleware. Using DDS as a communication mechanism, allows the video data to be transmitted in a data-centric manner with all the functionalities and advantages of the DDS Quality of Service (QoS) specifications. A demonstration video and detailed technical description is available on our website at <http://www.s2e-systems.com/our-projects/videooverdds/>
This software should build with cargo on Windows and Linux. Additionally a demonstration for Android is available under the android directory. A publisher app and a subscriber app are provided. With that it is possible to stream video from an Android phone to for example a Windows PC and vice versa.

## Dependencies ##

The project is developed using only open source and cross platform dependencies.

- Dust DDS (v 0.10.0)
  - <https://github.com/s2e-systems/dust-dds>
- GStreamer development and runtime (v 1.24.4)
  - <https://gstreamer.freedesktop.org/download>

Other version of these components probably work as well.

## More information ##

If you would like to use this software, have any questions, or want to know more about S2E Software Systems B.V. you can visit our website at <http://www.s2e-systems.com>


