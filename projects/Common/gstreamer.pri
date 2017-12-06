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

linux-arm-gnueabihf-g++ {
    GSTREAMER_DIR = $$clean_path($$[QT_SYSROOT]/opt/gstreamer)
    message(Serching GStreamer for linux-arm-gnueabihf-g++ at: $$GSTREAMER_DIR)
} else {
    GSTREAMER_DIR = $$clean_path($$(GSTREAMER_1_0_ROOT_X86_64))
    message(Serching GStreamer at: $$GSTREAMER_DIR)
}

GSTREAMER_LIBS += -lglib-2.0 -lgobject-2.0 -lgstreamer-1.0 -lgstbase-1.0 -lgstapp-1.0 -lgstvideo-1.0 #-lgstcodecparsers-1.0
GSTREAMER_LIBS += -L$$GSTREAMER_DIR/lib
GSTREAMER_INCLUDEPATH += \
    $$GSTREAMER_DIR/lib/gstreamer-1.0/include \
    $$GSTREAMER_DIR/include \
    $$GSTREAMER_DIR/include/gstreamer-1.0 \
    $$GSTREAMER_DIR/include/glib-2.0

linux-arm-gnueabihf-g++ {
    GSTREAMER_INCLUDEPATH += $$clean_path($$[QT_SYSROOT]/usr/lib/arm-linux-gnueabihf/glib-2.0/include)
    GSTREAMER_INCLUDEPATH += $$clean_path($$[QT_SYSROOT]/usr/include/glib-2.0)
} else {
    GSTREAMER_INCLUDEPATH += $$GSTREAMER_DIR/lib/glib-2.0/include
}

message(GSTREAMER_Dir: $$GSTREAMER_DIR)
message(GSTREAMER_INCLUDEPATH: $$GSTREAMER_INCLUDEPATH)
message(GSTREAMER_LIBS: $$GSTREAMER_LIBS)

LIBS += $$GSTREAMER_LIBS
INCLUDEPATH += $$GSTREAMER_INCLUDEPATH

