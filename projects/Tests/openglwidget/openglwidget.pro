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

message("TEST project: VideoWidget")

TARGET = VideoWidgetTest

COMMON_DIR = ../../Common


TEMPLATE = app

QT += core gui widgets #testlib #openglextensions

#DEFINES += EGL_EGLEXT_PROTOTYPES

#########
# Sources, Headers

INCLUDEPATH += $$COMMON_DIR

SOURCES += main.cpp \
    testvideowidget.cpp \
    $$COMMON_DIR/videowidgetgles2.cpp

HEADERS += \
    testvideowidget.h

# For Raspberry Pi deployment
target.path = /home/pi
INSTALLS += target
