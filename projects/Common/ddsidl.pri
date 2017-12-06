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

#
# DDS IDL extra-compiler for handling files specified in the DDSIDLSOURCES variable
#

# Make sure we include the generated header files path
INCLUDEPATH += $$OUT_PWD

{
    # The decl compiler is used to generate the command and the first source file
    # and the impl compiler is used to include the second generated source file
    ddsidl.name = DDS IDL ${QMAKE_FILE_BASE}
    ddsidl.input = DDSIDLSOURCES
    ddsidl.variable_out = GENERATED_SOURCES
    ddsidl.dependency_type = TYPE_C
    ddsidl.CONFIG = target_predeps
    ddsidl.commands = idlpp -S -l isocpp2 ${QMAKE_FILE_IN}
    ddsidl.output = $${absolute_path(${QMAKE_FILE_BASE}.cpp,$$OUT_PWD)}

    ddsidl.clean = ${QMAKE_FILE_BASE}.cpp \
                   ${QMAKE_FILE_BASE}.h \
                   ${QMAKE_FILE_BASE}_DCPS.hpp \
                   ${QMAKE_FILE_BASE}SplDcps.cpp \
                   ${QMAKE_FILE_BASE}SplDcps.h

    silent:ddsidl.commands = @echo DDS IDL generation ${QMAKE_FILE_IN} && $$ddsidl.commands


    ddsidl_h.input = DDSIDLSOURCES
    ddsidl_h.output = $${absolute_path(${QMAKE_FILE_BASE}.h, $$OUT_PWD)}
    ddsidl_h.variable_out = HEADERS
    ddsidl_h.CONFIG = no_link
    ddsidl_h.depends = $${absolute_path(${QMAKE_FILE_BASE}.cpp, $$OUT_PWD)}
    ddsidl_h.command = $$escape_expand(\\n)   # force creation of rule

    ddsidl_DCPS_hpp.input = DDSIDLSOURCES
    ddsidl_DCPS_hpp.output = $${absolute_path(${QMAKE_FILE_BASE}_DCPS.hpp,$$OUT_PWD)}
    ddsidl_DCPS_hpp.variable_out = HEADERS
    ddsidl_DCPS_hpp.CONFIG = no_link
    ddsidl_DCPS_hpp.depends = $${absolute_path(${QMAKE_FILE_BASE}.cpp, $$OUT_PWD)}
    ddsidl_DCPS_hpp.command = $$escape_expand(\\n)   # force creation of rule

    ddsidlSplDcps_cpp.input = DDSIDLSOURCES
    ddsidlSplDcps_cpp.output = $${absolute_path(${QMAKE_FILE_BASE}SplDcps.cpp, $$OUT_PWD)}
    ddsidlSplDcps_cpp.variable_out = GENERATED_SOURCES
    ddsidlSplDcps_cpp.command = $$escape_expand(\\n)   # force creation of rule
    ddsidlSplDcps_cpp.depends = $${absolute_path(${QMAKE_FILE_BASE}.cpp, $$OUT_PWD)}

    ddsidlSplDcps_h.input = DDSIDLSOURCES
    ddsidlSplDcps_h.output = $${absolute_path(${QMAKE_FILE_BASE}SplDcps.h, $$OUT_PWD)}
    ddsidlSplDcps_h.variable_out = GENERATED_SOURCES
    ddsidlSplDcps_h.command = $$escape_expand(\\n)   # force creation of rule
    ddsidlSplDcps_h.depends = $${absolute_path(${QMAKE_FILE_BASE}.cpp, $$OUT_PWD)}

    QMAKE_EXTRA_COMPILERS += ddsidl ddsidl_h ddsidl_DCPS_hpp ddsidlSplDcps_cpp ddsidlSplDcps_h
}
