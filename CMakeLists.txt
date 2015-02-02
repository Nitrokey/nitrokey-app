CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(CryptoStickGUI)
SET(PROJECT_VERSION "1.0") 
#ADD_DEFINITIONS(-DSEAFILE_CLIENT_VERSION=${PROJECT_VERSION}) 

INCLUDE(FindPkgConfig)

IF (NOT (${CMAKE_BUILD_TYPE} MATCHES Release))
  SET(CMAKE_BUILD_TYPE Debug) 
ENDIF()
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}") 


# QT configuration
FIND_PACKAGE(Qt5Core)
FIND_PACKAGE(Qt5Gui)
FIND_PACKAGE(Qt5Widgets)


# Moc files
SET(moc_headers
    aboutdialog.h
    base32.h
    command.h
    crc32.h
    nitrokey-applet.h
    device.h
    gui.h
    hidapi.h
    hotpdialog.h
    hotpslot.h
    inttypes.h
    mainwindow.h
    mcvs-wrapper.h
    passworddialog.h
    passwordsafedialog.h
    pindialog.h
    response.h
    securitydialog.h
    sleep.h
    splash.h
    stick20changepassworddialog.h
    stick20debugdialog.h
    stick20dialog.h
    stick20hiddenvolumedialog.h
    stick20hid.h
    stick20infodialog.h
    stick20lockfirmwaredialog.h
    stick20matrixpassworddialog.h
    stick20responsedialog.h
    stick20setup.h
    stick20updatedialog.h
    stick20window.h
    totpslot.h
)


# Ui files
SET(ui_files
    aboutdialog.ui
    hotpdialog.ui
    mainwindow.ui
    passworddialog.ui
    passwordsafedialog.ui
    pindialog.ui
    securitydialog.ui
    stick20changepassworddialog.ui
    stick20debugdialog.ui
    stick20dialog.ui
    stick20hiddenvolumedialog.ui
    stick20infodialog.ui
    stick20lockfirmwaredialog.ui
    stick20matrixpassworddialog.ui
    stick20responsedialog.ui
    stick20setup.ui
    stick20updatedialog.ui
    stick20window.ui
)


# Resource files
SET(qrc_files
    resources.qrc
)


# preprocess ui files
QT5_WRAP_UI(ui_output ${ui_files})

# meta object compliation(moc)
QT5_WRAP_CPP(moc_output ${moc_headers})

# resources files
QT5_ADD_RESOURCES(resources_ouput ${qrc_files})


FIND_PACKAGE(PkgConfig REQUIRED)
SET(LIBUSB_INCLUDE_DIRS "/usr/include/libusb-1.0")

# c/cpp sources
SET(nitrokey_app_sources
    aboutdialog.cpp
    base32.cpp
    command.cpp
    crc32.cpp
    nitrokey-applet.cpp
    device.cpp
    gui.cpp
    hotpdialog.cpp
    hotpslot.cpp
    main.cpp
    mainwindow.cpp
    passworddialog.cpp
    passwordsafedialog.cpp
    pindialog.cpp
    response.cpp
    securitydialog.cpp
    splash.cpp
    stick20changepassworddialog.cpp
    stick20debugdialog.cpp
    stick20dialog.cpp
    stick20hiddenvolumedialog.cpp
    stick20infodialog.cpp
    stick20lockfirmwaredialog.cpp
    stick20matrixpassworddialog.cpp
    stick20responsedialog.cpp
    stick20setup.cpp
    stick20updatedialog.cpp
    stick20window.cpp
    totpslot.cpp
    stick20hid.c
    hid_libusb.c
    ${platform_specific_sources}
)

IF (WIN32)
    SET( platform_specific_sources hid_win.c)
ELSEIF(APPLE)
    SET( platform_specific_sources hid_mac.c)
    SET( platform_specific_libs "-framework IOKit -framework CoreFoundation")
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    SET(EXTRA_LIBS pthread usb-1.0)
ENDIF()

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}
    ${QT_INCLUDE_DIR}
    ${LIBUSB_INCLUDE_DIRS}
)

LINK_DIRECTORIES(
    ${QT_LIBRARY_DIR}
    ${LIBUSB_LIBRARY_DIRS}
)


# Freedesktop files
if(NOT WIN32)
install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/data/icons/16x16
  ${CMAKE_SOURCE_DIR}/data/icons/22x22
  ${CMAKE_SOURCE_DIR}/data/icons/24x24
  ${CMAKE_SOURCE_DIR}/data/icons/32x32
  ${CMAKE_SOURCE_DIR}/data/icons/48x48
  ${CMAKE_SOURCE_DIR}/data/icons/128x128
  ${CMAKE_SOURCE_DIR}/data/icons/scalable
  DESTINATION share/icons/hicolor
)

install(FILES
  ${CMAKE_SOURCE_DIR}/data/nitrokey-app.desktop
  DESTINATION share/applications
)

install(FILES
  ${CMAKE_SOURCE_DIR}/data/icons/128x128/apps/nitrokey-app.png
  DESTINATION share/pixmaps
)

endif()


ADD_EXECUTABLE(nitrokey-app ${GUI_TYPE}
  ${nitrokey_app_sources}
  ${moc_output}
  ${ui_output}
  ${resources_ouput}
)

INSTALL(TARGETS nitrokey-app DESTINATION bin)

TARGET_LINK_LIBRARIES(nitrokey-app
  ${QT_LIBRARIES}
  ${LIBUSB_LIBRARIES}
  ${EXTRA_LIBS}
  ${platform_specific_libs}
)

qt5_use_modules( nitrokey-app Core Gui Widgets)

# Packaging
SET(CPACK_GENERATOR
    "DEB;RPM")

SET(CPACK_PACKAGE_NAME "nitrokey-app" )
SET(CPACK_PACKAGE_VERSION "1.0" )
SET(CPACK_PACKAGE_ARCHITECTURE )
SET(CPACK_PACKAGE_DEPENDS "" )
SET(CPACK_PACKAGE_DESCRIPTION  "Application to manage your Nitrokey")

# Deb
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Georgios Gkitsas <george@nitrokey.com>" )
SET(CPACK_PACKAGE_PRIORITY "optional")
SET(CPACK_PACKAGE_SECTION "utils")

# Rpm


INCLUDE(CPack)
