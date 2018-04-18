# Created by and for Qt Creator. This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

CONFIG   += c++14 shared debug


TEMPLATE     = lib
TARGET = nitrokey

VERSION = 3.3
QMAKE_TARGET_COMPANY = Nitrokey
QMAKE_TARGET_PRODUCT = libnitrokey
QMAKE_TARGET_DESCRIPTION = Communicate with Nitrokey stick devices in a clean and easy manner
QMAKE_TARGET_COPYRIGHT = Copyright (c) 2015-2018 Nitrokey UG

HEADERS = \
   $$PWD/libnitrokey/hidapi/hidapi.h \
   $$PWD/libnitrokey/command.h \
   $$PWD/libnitrokey/command_id.h \
   $$PWD/libnitrokey/CommandFailedException.h \
   $$PWD/libnitrokey/cxx_semantics.h \
   $$PWD/libnitrokey/device.h \
   $$PWD/libnitrokey/device_proto.h \
   $$PWD/libnitrokey/DeviceCommunicationExceptions.h \
   $$PWD/libnitrokey/dissect.h \
   $$PWD/libnitrokey/LibraryException.h \
   $$PWD/libnitrokey/log.h \
   $$PWD/libnitrokey/LongOperationInProgressException.h \
   $$PWD/libnitrokey/misc.h \
   $$PWD/libnitrokey/NitrokeyManager.h \
   $$PWD/libnitrokey/stick10_commands.h \
   $$PWD/libnitrokey/stick10_commands_0.8.h \
   $$PWD/libnitrokey/stick20_commands.h \
   $$PWD/NK_C_API.h


SOURCES = \
   $$PWD/command_id.cc \
   $$PWD/device.cc \
   $$PWD/DeviceCommunicationExceptions.cpp \
   $$PWD/log.cc \
   $$PWD/misc.cc \
   $$PWD/NitrokeyManager.cc \
   $$PWD/NK_C_API.cc


tests {
    SOURCES += \
       $$PWD/unittest/catch_main.cpp \
       $$PWD/unittest/test.cc \
       $$PWD/unittest/test2.cc \
       $$PWD/unittest/test3.cc \
       $$PWD/unittest/test_C_API.cpp \
       $$PWD/unittest/test_HOTP.cc
}

unix:!macx{
#   SOURCES += $$PWD/hidapi/linux/hid.c
    LIBS += -lhidapi-libusb
}

macx{
    SOURCES += $$PWD/hidapi/mac/hid.c
    LIBS+= -framework IOKit -framework CoreFoundation
}

win32 {
    SOURCES += $$PWD/hidapi/windows/hid.c
    LIBS += -lsetupapi
}

INCLUDEPATH = \
    $$PWD/. \
    $$PWD/hidapi/hidapi \
    $$PWD/libnitrokey \
    $$PWD/libnitrokey/hidapi \
    $$PWD/unittest \
    $$PWD/unittest/Catch/single_include

#DEFINES = 

unix:!macx{
        # Install rules for QMake (CMake is preffered though)
        udevrules.path = $$system(pkg-config --variable=udevdir udev)
        isEmpty(udevrules.path){
            udevrules.path = "/lib/udev/"
            message("Could not detect path for udev rules - setting default: " $$udevrules.path)
        }
        udevrules.path = $$udevrules.path"/rules.d"
        udevrules.files = $$PWD/"data/41-nitrokey.rules"
        message ($$udevrules.files)
        INSTALLS +=udevrules

        headers.files = $$HEADERS
        headers.path = /usr/local/include/libnitrokey/
        INSTALLS += headers

        libbin.path = /usr/local/lib
        INSTALLS += libbin
}
