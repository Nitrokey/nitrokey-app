# message("CONFIG: $${CONFIG}")

include(libnitrokey/libnitrokey.pro)

CONFIG   += qt c++14
QT       += core gui widgets svg concurrent

isEmpty(PREFIX) {
    PREFIX = /usr/local
}

target.path = $$PREFIX/bin
desktop.path = $$PREFIX/share/applications
desktop.files = data/nitrokey-app.desktop
icons.files = data/icons/*
icons.path = $$PREFIX/share/icons/
INSTALLS = target desktop icons

sources.files = qss


TARGET = nitrokey-app
TEMPLATE = app

VERSION = 1.5.0
VERSION_STR = 1.5.0
QMAKE_TARGET_COMPANY = Nitrokey
QMAKE_TARGET_PRODUCT = Nitrokey App
QMAKE_TARGET_DESCRIPTION = Nitrokey Device Manager

QMAKE_TARGET_COPYRIGHT = Copyright (c) 2012-2022 Nitrokey GmbH


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
            $${SRCDIR}/GUI/ManageWindow.cpp \
            $${SRCDIR}/libada.cpp \
    src/ui/licensedialog.cpp \
    src/GUI/graphicstools.cpp


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
    src/ui/licensedialog.h \
    src/GUI/graphicstools.h




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


win32:RC_ICONS = images/icon/nitrokey-app-icon-256-red.ico

macx{
    ICON= images/CS_icon.icns
    QMAKE_INFO_PLIST = Info.plist

    # PLUGINS += iconengines/libqsvgicon.dylib
#    QTPLUGIN += qsvgicon libqsvgicon
#    CONFIG += import_plugins
}


OTHER_FILES +=

RESOURCES += \
    $${ROOTDIR}/resources.qrc

TRANSLATIONS += i18n/nitrokey_de_DE.ts \
                i18n/nitrokey_arabic.ts \
                i18n/nitrokey_en.ts \
                i18n/nitrokey_pl.ts \
                i18n/nitrokey_fr.ts \
                i18n/nitrokey_it.ts


DEFINES += GIT_VERSION=\"\\\"$$system(git describe --abbrev=4 --always)\\\"\" GUI_VERSION="\\\"$${VERSION_STR}\\\""
message($$DEFINES)
