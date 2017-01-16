#-------------------------------------------------
#
# Project created by QtCreator 2014-07-06T17:56:57
#
#-------------------------------------------------

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
UIDIR=$${ROOTDIR}/ui
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
            $${SRCUIDIR}/stick20-response-task.cpp \
            $${SRCUIDIR}/stick20lockfirmwaredialog.cpp \
            $${SRCUIDIR}/stick20hiddenvolumedialog.cpp \
            $${SRCUIDIR}/stick20debugdialog.cpp \
            $${SRCUIDIR}/stick20changepassworddialog.cpp \
            $${SRCUIDIR}/pindialog.cpp \
            $${UTILSDIR}/base32.cpp \
            $${SRCUIDIR}/aboutdialog.cpp \
            $${SRCUIDIR}/securitydialog.cpp \
            $${SRCUIDIR}/nitrokey-applet.cpp \
            $${SRCDIR}/core/SecureString.cpp \
            $${SRCDIR}/GUI/Tray.cpp \
            $${SRCDIR}/libada.cpp


HEADERS  += $${SRCUIDIR}/mainwindow.h \
            $${SRCUIDIR}/stick20updatedialog.h \
            $${SRCUIDIR}/stick20responsedialog.h \
            $${SRCUIDIR}/stick20-response-task.h \
            $${SRCUIDIR}/stick20lockfirmwaredialog.h \
            $${SRCUIDIR}/stick20hiddenvolumedialog.h \
            $${SRCUIDIR}/stick20debugdialog.h \
            $${SRCUIDIR}/stick20changepassworddialog.h \
            $${UTILSDIR}/sleep.h \
            $${SRCUIDIR}/pindialog.h \
            $${SRCDIR}/hotpslot.h \
            $${SRCDIR}/systemutils.h \
            $${UTILSDIR}/base32.h \
            $${SRCUIDIR}/aboutdialog.h \
            $${SRCUIDIR}/securitydialog.h \
            $${SRCDIR}/mcvs-wrapper.h \
            $${SRCUIDIR}/nitrokey-applet.h \
            $${SRCDIR}/core/SecureString.h \
            $${SRCDIR}/libada.h \
            $${SRCDIR}/GUI/Tray.h

win32 {
}

macx{
}

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
            $${UIDIR}/securitydialog.ui

INCLUDEPATH +=  $${SRCDIR} \
                $${SRCUIDIR} \
                $${UTILSDIR} \
                $${COREDIR} \
                $${GUIDIR}

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
    LIBS  = -lrt -lpthread
}

LIBS += -lnitrokey -L libnitrokey/build/

OTHER_FILES +=

RESOURCES += \
    resources.qrc

TRANSLATIONS += i18n/nitrokey_de_DE.ts \
                i18n/nitrokey_ar.ts
                i18n/nitrokey_en.ts
