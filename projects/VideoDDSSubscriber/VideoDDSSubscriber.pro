# Copyright 2017 S2E Software, Systems and Control 
#  
# Licensed under the Apache License, Version 2.0 the "License"; 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at 
#  
#    http://www.apache.org/licenses/LICENSE-2.0 
#  
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions and 
# limitations under the License.

message("Video DDS Subscriber project")

TARGET = VideoDDSSubscriber

COMMON_DIR = ../Common

include($$COMMON_DIR/ddsidl.pri)
include($$COMMON_DIR/opensplice.pri)
include($$COMMON_DIR/gstreamer.pri)

TEMPLATE = app
CONFIG += c++11 ddsidl
CONFIG -= app_bundle

QT += core gui widgets

DEFINES += EGL_EGLEXT_PROTOTYPES

DDSIDLSOURCES += $$COMMON_DIR/VideoDDS.idl

# Force qmake to not transform an absolute path to relative path
# this solved a header not found error during compilation
QMAKE_PROJECT_DEPTH = 0

#########
# Sources, Headers

INCLUDEPATH += $$COMMON_DIR

SOURCES += main.cpp \
    videoddssubscriber.cpp \
    videolistener.cpp \
    $$COMMON_DIR/pipeline.cpp \
    $$COMMON_DIR/qtgstreamer.cpp \
    $$COMMON_DIR/videowidgetgles2.cpp \
    $$COMMON_DIR/videowidgetgst.cpp

HEADERS += \
    videoddssubscriber.h \
    videolistener.h \
    $$COMMON_DIR/pipeline.h \
    $$COMMON_DIR/qtgstreamer.h \
    $$COMMON_DIR/videowidgetgles2.h \
    $$COMMON_DIR/videowidgetgst.h

# For Raspberry Pi deployment
target.path = /home/pi
INSTALLS += target
