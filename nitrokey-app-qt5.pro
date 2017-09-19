

message("CONFIG: $${CONFIG}")

CONFIG   += qt c++14
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
UIDIR=$${ROOTDIR}/src/ui
SRCDIR=$${ROOTDIR}/src
SRCUIDIR=$${SRCDIR}/ui
UTILSDIR=$${SRCDIR}/utils
COREDIR=$${SRCDIR}/core
GUIDIR=$${SRCDIR}/GUI


SOURCES +=  $${SRCDIR}/main.cpp\
            $${SRCUIDIR}/mainwindow.cpp \
            $${SRCDIR}/hotpslot.cpp \
            $${SRCUIDIR}/stick20updatedialog.cpp \
            $${SRCUIDIR}/stick20responsedialog.cpp \
            $${SRCUIDIR}/stick20lockfirmwaredialog.cpp \
            $${SRCUIDIR}/stick20hiddenvolumedialog.cpp \
            $${SRCUIDIR}/stick20debugdialog.cpp \
            $${SRCUIDIR}/stick20changepassworddialog.cpp \
            $${SRCUIDIR}/pindialog.cpp \
            $${SRCUIDIR}/aboutdialog.cpp \
            $${SRCUIDIR}/securitydialog.cpp \
            $${SRCUIDIR}/nitrokey-applet.cpp \
            $${SRCDIR}/core/SecureString.cpp \
            $${SRCDIR}/core/ThreadWorker.cpp \
            $${SRCDIR}/GUI/Tray.cpp \
            $${SRCDIR}/GUI/Clipboard.cpp \
            $${SRCDIR}/GUI/Authentication.cpp \
            $${SRCDIR}/GUI/StorageActions.cpp \
            $${SRCDIR}/libada.cpp \
    src/ui/licensedialog.cpp


HEADERS  += $${SRCUIDIR}/mainwindow.h \
            $${SRCUIDIR}/stick20updatedialog.h \
            $${SRCUIDIR}/stick20responsedialog.h \
            $${SRCUIDIR}/stick20lockfirmwaredialog.h \
            $${SRCUIDIR}/stick20hiddenvolumedialog.h \
            $${SRCUIDIR}/stick20debugdialog.h \
            $${SRCUIDIR}/stick20changepassworddialog.h \
            $${UTILSDIR}/sleep.h \
            $${SRCUIDIR}/pindialog.h \
            $${SRCDIR}/hotpslot.h \
            $${SRCDIR}/systemutils.h \
            $${SRCUIDIR}/aboutdialog.h \
            $${SRCUIDIR}/securitydialog.h \
            $${SRCDIR}/mcvs-wrapper.h \
            $${SRCUIDIR}/nitrokey-applet.h \
            $${SRCDIR}/core/SecureString.h \
            $${SRCDIR}/core/ThreadWorker.h \
            $${SRCDIR}/GUI/Clipboard.h \
            $${SRCDIR}/GUI/Authentication.h \
            $${SRCDIR}/libada.h \
            $${SRCDIR}/GUI/Tray.h \
            $${SRCDIR}/GUI/StorageActions.h \
    src/ui/licensedialog.h




unix:!macx{
    SOURCES += $${SRCDIR}/systemutils.cpp
    HEADERS += $${SRCDIR}/systemutils.h
}


FORMS +=    $${UIDIR}/mainwindow.ui \
            $${UIDIR}/stick20updatedialog.ui \
            $${UIDIR}/stick20responsedialog.ui \
            $${UIDIR}/stick20lockfirmwaredialog.ui \
            $${UIDIR}/stick20hiddenvolumedialog.ui \
            $${UIDIR}/stick20debugdialog.ui \
            $${UIDIR}/stick20changepassworddialog.ui \
            $${UIDIR}/passworddialog.ui \
            $${UIDIR}/pindialog.ui \
            $${UIDIR}/aboutdialog.ui \
            $${UIDIR}/securitydialog.ui \
    src/ui/licensedialog.ui

INCLUDEPATH +=  $${SRCDIR} \
                $${SRCUIDIR} \
                $${UTILSDIR} \
                $${COREDIR} \
		        $${ROOTDIR}/3rdparty/cppcodec \
                $${GUIDIR}

LIBNITROKEY= -lnitrokey-static
enable_log {
    message("enable_log : enabled")
    LIBNITROKEY= -lnitrokey-static-log
#    CONFIG += console
}


unix{
    LIBS += $${LIBNITROKEY} -L$${ROOTDIR}/libnitrokey/build
}
unix:!macx{
    LIBS += -lhidapi-libusb
}

# TODO make second binary - debug - linked with libnitrokey-log
win32 {
    INCLUDEPATH += $${ROOTDIR}/libnitrokey/hidapi/hidapi/
    #TODO add hidapi sources to libnitrokey instead
    SOURCES += $${ROOTDIR}/libnitrokey/hidapi/windows/hid.c
    LIBS= -lsetupapi -lhid
    LIBS += $${LIBNITROKEY} -L$${ROOTDIR}/libnitrokey/build
    RC_FILE=appico.rc
}

macx{
    INCLUDEPATH += $${ROOTDIR}/libnitrokey/hidapi/hidapi/
    #TODO add hidapi sources to libnitrokey instead
    SOURCES += $${ROOTDIR}/libnitrokey/hidapi/mac/hid.c
    LIBS+=-framework IOKit -framework CoreFoundation
    ICON= images/CS_icon.icns
    QMAKE_INFO_PLIST = Info.plist
}


OTHER_FILES +=

RESOURCES += \
    $${ROOTDIR}/resources.qrc

TRANSLATIONS += i18n/nitrokey_de_DE.ts \
                i18n/nitrokey_arabic.ts \
                i18n/nitrokey_en.ts
                i18n/nitrokey_fr.ts
