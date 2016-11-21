#-------------------------------------------------
#
# Project created by QtCreator 2014-07-06T17:56:57
#
#-------------------------------------------------

CONFIG   += qt
QT       += core gui

target.path = /usr/local/bin
desktop.path = /usr/share/applications
desktop.files += nitrokey-app.desktop

INSTALLS += target desktop

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

sources.files = qss

TARGET = nitrokey-app
TEMPLATE = app

ROOTDIR=$$PWD
UIDIR=$${ROOTDIR}/ui
SRCDIR=$${ROOTDIR}/src
SRCUIDIR=$${SRCDIR}/ui
UTILSDIR=$${SRCDIR}/utils


SOURCES +=  $${SRCDIR}/main.cpp\
            $${SRCUIDIR}/mainwindow.cpp \
            $${SRCDIR}/hotpslot.cpp \
            $${SRCUIDIR}/stick20window.cpp \
            $${SRCUIDIR}/stick20updatedialog.cpp \
            $${SRCUIDIR}/stick20setup.cpp \
            $${SRCUIDIR}/stick20responsedialog.cpp \
            $${SRCUIDIR}/stick20-response-task.cpp \
            $${SRCUIDIR}/stick20matrixpassworddialog.cpp \
            $${SRCUIDIR}/stick20lockfirmwaredialog.cpp \
            $${SRCUIDIR}/stick20infodialog.cpp \
            $${SRCUIDIR}/stick20hiddenvolumedialog.cpp \
            $${SRCUIDIR}/stick20dialog.cpp \
            $${SRCUIDIR}/stick20debugdialog.cpp \
            $${SRCUIDIR}/stick20changepassworddialog.cpp \
            $${SRCDIR}/response.cpp \
            $${SRCUIDIR}/passworddialog.cpp \
            $${SRCUIDIR}/pindialog.cpp \
            $${SRCUIDIR}/hotpdialog.cpp \
            $${SRCDIR}/device.cpp \
            $${UTILSDIR}/crc32.cpp \
            $${SRCDIR}/command.cpp \
            $${UTILSDIR}/base32.cpp \
            $${SRCUIDIR}/aboutdialog.cpp \
            $${UTILSDIR}/stick20hid.c \
            $${SRCUIDIR}/passwordsafedialog.cpp \
            $${SRCUIDIR}/securitydialog.cpp \
            $${SRCUIDIR}/nitrokey-applet.cpp

win32 {
    SOURCES +=   $${UTILSDIR}/hid_win.c
}

macx{
    SOURCES +=   $${UTILSDIR}/hid_mac.c
}

unix:!macx{
    SOURCES +=   $${UTILSDIR}/hid_libusb.c \
            $${SRCDIR}/systemutils.cpp
}

HEADERS  += $${SRCUIDIR}/mainwindow.h \
            $${SRCUIDIR}/stick20window.h \
            $${SRCUIDIR}/stick20updatedialog.h \
            $${SRCUIDIR}/stick20setup.h \
            $${SRCUIDIR}/stick20responsedialog.h \
            $${SRCUIDIR}/stick20-response-task.h \
            $${SRCUIDIR}/stick20matrixpassworddialog.h \
            $${SRCUIDIR}/stick20lockfirmwaredialog.h \
            $${SRCUIDIR}/stick20infodialog.h \
            $${SRCUIDIR}/stick20hiddenvolumedialog.h \
            $${UTILSDIR}/stick20hid.h \
            $${SRCUIDIR}/stick20dialog.h \
            $${SRCUIDIR}/stick20debugdialog.h \
            $${SRCUIDIR}/stick20changepassworddialog.h \
            $${UTILSDIR}/sleep.h \
            $${SRCDIR}/response.h \
            $${SRCUIDIR}/passworddialog.h \
            $${SRCUIDIR}/pindialog.h \
            $${SRCDIR}/inttypes.h \
            $${SRCDIR}/hotpslot.h \
            $${SRCUIDIR}/hotpdialog.h \
            $${UTILSDIR}/hidapi.h \
            $${SRCDIR}/device.h \
            $${SRCDIR}/systemutils.h \
            $${UTILSDIR}/crc32.h \
            $${SRCDIR}/command.h \
            $${UTILSDIR}/base32.h \
            $${SRCUIDIR}/aboutdialog.h \
            $${SRCUIDIR}/passwordsafedialog.h \
            $${SRCUIDIR}/securitydialog.h \
            $${SRCDIR}/mcvs-wrapper.h \
            $${SRCUIDIR}/nitrokey-applet.h

FORMS +=    $${UIDIR}/mainwindow.ui \
            $${UIDIR}/stick20window.ui \
            $${UIDIR}/stick20updatedialog.ui \
            $${UIDIR}/stick20setup.ui \
            $${UIDIR}/stick20responsedialog.ui \
            $${UIDIR}/stick20matrixpassworddialog.ui \
            $${UIDIR}/stick20lockfirmwaredialog.ui \
            $${UIDIR}/stick20infodialog.ui \
            $${UIDIR}/stick20hiddenvolumedialog.ui \
            $${UIDIR}/stick20dialog.ui \
            $${UIDIR}/stick20debugdialog.ui \
            $${UIDIR}/stick20changepassworddialog.ui \
            $${UIDIR}/passworddialog.ui \
            $${UIDIR}/pindialog.ui \
            $${UIDIR}/hotpdialog.ui \
            $${UIDIR}/aboutdialog.ui \
            $${UIDIR}/passwordsafedialog.ui \
            $${UIDIR}/securitydialog.ui

INCLUDEPATH +=  $${SRCDIR} \
                $${SRCUIDIR} \
                $${UTILSDIR}

win32{
    LIBS= -lsetupapi
    RC_FILE=appico.rc
}

macx{
    LIBS=-framework IOKit -framework CoreFoundation
    ICON= images/CS_icon.icns
    QMAKE_INFO_PLIST = Info.plist
}

unix:!macx{
    LIBS  = `pkg-config libusb-1.0 --libs` -lrt -lpthread
    INCLUDEPATH += /usr/include/libusb-1.0

    INCLUDEPATH += /usr/include/libappindicator-0.1 \
        /usr/include/gtk-2.0 \
        /usr/lib/gtk-2.0/include \
        /usr/include/glib-2.0 \
        /usr/lib/glib-2.0/include \
        /usr/include/cairo \
        /usr/include/atk-1.0 \
        /usr/include/pango-1.0

    LIBS += -L/usr/lib -lappindicator -lnotify
    CONFIG += link_pkgconfig
    PKGCONFIG = gtk+-2.0
}

OTHER_FILES +=

RESOURCES += \
    resources.qrc

TRANSLATIONS += i18n/nitrokey_de_DE.ts \
                i18n/nitrokey_en.ts
