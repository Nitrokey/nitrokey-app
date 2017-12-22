# Created by and for Qt Creator. This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

CONFIG   += c++14 shared debug

TEMPLATE     = lib
TARGET = nitrokey

HEADERS = \
   $$PWD/hidapi/hidapi/hidapi.h \
   $$PWD/include/command.h \
   $$PWD/include/command_id.h \
   $$PWD/include/CommandFailedException.h \
   $$PWD/include/cxx_semantics.h \
   $$PWD/include/device.h \
   $$PWD/include/device_proto.h \
   $$PWD/include/DeviceCommunicationExceptions.h \
   $$PWD/include/dissect.h \
   $$PWD/include/inttypes.h \
   $$PWD/include/LibraryException.h \
   $$PWD/include/log.h \
   $$PWD/include/LongOperationInProgressException.h \
   $$PWD/include/misc.h \
   $$PWD/include/NitrokeyManager.h \
   $$PWD/include/stick10_commands.h \
   $$PWD/include/stick10_commands_0.8.h \
   $$PWD/include/stick20_commands.h \
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
    $$PWD/include \
    $$PWD/include/hidapi \
    $$PWD/unittest \
    $$PWD/unittest/Catch/single_include

#DEFINES = 

