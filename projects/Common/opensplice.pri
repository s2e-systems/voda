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
    OSPL_DIR = $$clean_path($$[QT_SYSROOT]/opt/ospl)
    message(Serching OpenSplice at linux-arm-gnueabihf-g++ at: $$OSPL_DIR)
} else {
    OSPL_DIR = $$clean_path($$(OSPL_HOME))
    message(Serching OpenSplice at: $$OSPL_DIR)
}

OSPL_LIBS +=  -lddskernel

CONFIG( debug, debug|release ) {
    # debug
    OSPL_LIBS += -ldcpsisocpp2d -ldcpssacppd
} else {
    # release
    OSPL_LIBS += -ldcpsisocpp2 -ldcpssacpp
}
OSPL_LIBS += -L$$OSPL_DIR/lib

OSPL_INCLUDEPATH += $$OSPL_DIR/include
OSPL_INCLUDEPATH += $$OSPL_DIR/include/sys
OSPL_INCLUDEPATH += $$OSPL_DIR/include/dcps/C++/isocpp2
OSPL_INCLUDEPATH += $$OSPL_DIR/include/dcps/C++/SACPP


message(OSPL_Dir: $$OSPL_DIR)
message(OSPL_INCLUDEPATH: $$OSPL_INCLUDEPATH)
message(OSPL_LIBS: $$OSPL_LIBS)


LIBS += $$OSPL_LIBS
INCLUDEPATH += $$OSPL_INCLUDEPATH
