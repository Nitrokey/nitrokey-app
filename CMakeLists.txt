CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(NitrokeyApp)
SET(PROJECT_VERSION "0.5.0")
#ADD_DEFINITIONS(-DSEAFILE_CLIENT_VERSION=${PROJECT_VERSION}) 

OPTION(HAVE_LIBAPPINDICATOR "Compile support for libappindicator" YES)

INCLUDE(FindPkgConfig)

IF (NOT (${CMAKE_BUILD_TYPE} MATCHES Release))
  SET(CMAKE_BUILD_TYPE Debug) 
ENDIF()
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}") 


# QT configuration
FIND_PACKAGE(Qt5Core)
FIND_PACKAGE(Qt5Gui)
FIND_PACKAGE(Qt5Widgets)


# Directory structure

SET(ROOTDIR ${CMAKE_CURRENT_SOURCE_DIR})
SET(UIDIR ${ROOTDIR}/ui)
SET(SRCDIR ${ROOTDIR}/src)
SET(SRCUIDIR ${SRCDIR}/ui)
SET(UTILSDIR ${SRCDIR}/utils)

# Moc files
SET(moc_headers
    ${SRCUIDIR}/aboutdialog.h
    ${UTILSDIR}/base32.h
    ${SRCDIR}/command.h
    ${UTILSDIR}/crc32.h
    ${SRCUIDIR}/nitrokey-applet.h
    ${SRCDIR}/device.h
    ${SRCUIDIR}/hotpdialog.h
    ${SRCDIR}/hotpslot.h
    ${SRCDIR}/inttypes.h
    ${SRCUIDIR}/mainwindow.h
    ${SRCDIR}/mcvs-wrapper.h
    ${SRCUIDIR}/passworddialog.h
    ${SRCUIDIR}/passwordsafedialog.h
    ${SRCUIDIR}/pindialog.h
    ${SRCDIR}/response.h
    ${SRCUIDIR}/securitydialog.h
    ${UTILSDIR}/sleep.h
    ${SRCUIDIR}/stick20changepassworddialog.h
    ${SRCUIDIR}/stick20debugdialog.h
    ${SRCUIDIR}/stick20dialog.h
    ${SRCUIDIR}/stick20hiddenvolumedialog.h
    ${UTILSDIR}/stick20hid.h
    ${SRCUIDIR}/stick20infodialog.h
    ${SRCUIDIR}/stick20lockfirmwaredialog.h
    ${SRCUIDIR}/stick20matrixpassworddialog.h
    ${SRCUIDIR}/stick20responsedialog.h
    ${SRCUIDIR}/stick20-response-task.h
    ${SRCUIDIR}/stick20setup.h
    ${SRCUIDIR}/stick20updatedialog.h
    ${SRCUIDIR}/stick20window.h
    ${SRCDIR}/totpslot.h
)


# Ui files
SET(ui_files
    ${UIDIR}/aboutdialog.ui
    ${UIDIR}/hotpdialog.ui
    ${UIDIR}/mainwindow.ui
    ${UIDIR}/passworddialog.ui
    ${UIDIR}/passwordsafedialog.ui
    ${UIDIR}/pindialog.ui
    ${UIDIR}/securitydialog.ui
    ${UIDIR}/stick20changepassworddialog.ui
    ${UIDIR}/stick20debugdialog.ui
    ${UIDIR}/stick20dialog.ui
    ${UIDIR}/stick20hiddenvolumedialog.ui
    ${UIDIR}/stick20infodialog.ui
    ${UIDIR}/stick20lockfirmwaredialog.ui
    ${UIDIR}/stick20matrixpassworddialog.ui
    ${UIDIR}/stick20responsedialog.ui
    ${UIDIR}/stick20setup.ui
    ${UIDIR}/stick20updatedialog.ui
    ${UIDIR}/stick20window.ui
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

IF (WIN32)
    SET( platform_specific_sources hid_win.c)
ELSEIF(APPLE)
    SET( platform_specific_sources hid_mac.c)
    SET( platform_specific_libs "-framework IOKit -framework CoreFoundation")
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")

    SET( platform_specific_sources ${SRCDIR}/systemutils.cpp)
    pkg_check_modules(GTK2 gtk+-2.0)

    IF (GTK2_FOUND AND HAVE_LIBAPPINDICATOR)

      SET(GTK2_INCLUDE_DIRS
          ${GTK2_INCLUDE_DIRS}
          /usr/include/libappindicator-0.1
          /usr/include/gtk-2.0
          /usr/lib/gtk-2.0/include
          /usr/include/glib-2.0
          /usr/lib/glib-2.0/include
          /usr/include/cairo
          /usr/include/atk-1.0
          /usr/include/pango-1.0
      )

      SET(EXTRA_LIBS ${GTK2_LIBRARIES} appindicator notify)

      ADD_DEFINITIONS(${GTK2_CFLAGS_OTHER} -DHAVE_LIBAPPINDICATOR)

    ENDIF () # GTK2_FOUND AND HAVE_LIBAPPINDICATOR

    SET(EXTRA_LIBS ${EXTRA_LIBS} pthread usb-1.0)

    INCLUDE_DIRECTORIES(
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_BINARY_DIR}
      ${SRCDIR}
      ${UTILSDIR}
      ${SRCUIDIR}
      ${PROJECT_SOURCE_DIR}
      ${QT_INCLUDE_DIR}
      ${LIBUSB_INCLUDE_DIRS}
      ${GTK2_INCLUDE_DIRS}
    )

    LINK_DIRECTORIES(
      ${QT_LIBRARY_DIR}
      ${LIBUSB_LIBRARY_DIRS}
      ${GTK2_LIBRARY_DIRS}
    )

ENDIF () # WIN32

# c/cpp sources
SET(nitrokey_app_sources
    ${SRCUIDIR}/aboutdialog.cpp
    ${UTILSDIR}/base32.cpp
    ${SRCDIR}/command.cpp
    ${UTILSDIR}/crc32.cpp
    ${SRCUIDIR}/nitrokey-applet.cpp
    ${SRCDIR}/device.cpp
    ${SRCUIDIR}/hotpdialog.cpp
    ${SRCDIR}/hotpslot.cpp
    ${SRCDIR}/main.cpp
    ${SRCUIDIR}/mainwindow.cpp
    ${SRCUIDIR}/passworddialog.cpp
    ${SRCUIDIR}/passwordsafedialog.cpp
    ${SRCUIDIR}/pindialog.cpp
    ${SRCDIR}/response.cpp
    ${SRCUIDIR}/securitydialog.cpp
    ${SRCUIDIR}/stick20changepassworddialog.cpp
    ${SRCUIDIR}/stick20debugdialog.cpp
    ${SRCUIDIR}/stick20dialog.cpp
    ${SRCUIDIR}/stick20hiddenvolumedialog.cpp
    ${SRCUIDIR}/stick20infodialog.cpp
    ${SRCUIDIR}/stick20lockfirmwaredialog.cpp
    ${SRCUIDIR}/stick20matrixpassworddialog.cpp
    ${SRCUIDIR}/stick20responsedialog.cpp
    ${SRCUIDIR}/stick20-response-task.cpp
    ${SRCUIDIR}/stick20setup.cpp
    ${SRCUIDIR}/stick20updatedialog.cpp
    ${SRCUIDIR}/stick20window.cpp
    ${SRCDIR}/totpslot.cpp
    ${UTILSDIR}/stick20hid.c
    ${UTILSDIR}/hid_libusb.c
    ${platform_specific_sources}
)

# Freedesktop files
IF(NOT WIN32)
  install(DIRECTORY
      #      ${CMAKE_SOURCE_DIR}/data/icons/16x16
      #      ${CMAKE_SOURCE_DIR}/data/icons/22x22
      #      ${CMAKE_SOURCE_DIR}/data/icons/24x24
      #      ${CMAKE_SOURCE_DIR}/data/icons/32x32
      #      ${CMAKE_SOURCE_DIR}/data/icons/48x48
      #      ${CMAKE_SOURCE_DIR}/data/icons/128x128
    ${CMAKE_SOURCE_DIR}/data/icons/
    DESTINATION share/icons/
  )

  install(FILES
    ${CMAKE_SOURCE_DIR}/data/nitrokey-app.desktop
    DESTINATION share/applications
  )

  install(FILES
    ${CMAKE_SOURCE_DIR}/data/icons/hicolor/128x128/apps/nitrokey-app.png
    DESTINATION share/pixmaps
  )

  # Install Nitrokey udev rules
  install(FILES
    ${CMAKE_SOURCE_DIR}/data/40-nitrokey.rules
    DESTINATION /usr/lib/udev/rules.d
  )

  # Install autocompletion scripts
  install(FILES
    ${CMAKE_SOURCE_DIR}/data//bash-autocomplete/nitrokey-app
    DESTINATION /etc/bash_completion.d
  )

  install(FILES
    ${CMAKE_SOURCE_DIR}/po/de_DE/nitrokey-app.mo
    DESTINATION /usr/share/locale/de_DE/LC_MESSAGES
  )

  install(FILES
    ${CMAKE_SOURCE_DIR}/images/info.png
    ${CMAKE_SOURCE_DIR}/images/quit.png
    ${CMAKE_SOURCE_DIR}/images/safe_zahlenkreis.png
    ${CMAKE_SOURCE_DIR}/images/settings.png
    DESTINATION /usr/share/nitrokey
  )

ENDIF () # NOT WIN32


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
SET(CPACK_PACKAGE_VERSION ${PROJECT_VERSION} )
SET(CPACK_PACKAGE_DEPENDS "" )

# Deb
#execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE CPACK_PACKAGE_ARCHITECTURE)
execute_process(COMMAND uname -m OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION  "Use and manage your Nitrokey")

# We need to alter the architecture names as per distro rules
IF("${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}" MATCHES "i[3-6]86")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE i386)
ENDIF("${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}" MATCHES "i[3-6]86")
IF("${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}" MATCHES "x86_64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
ENDIF("${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}" MATCHES "x86_64")

SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Mateusz Zalega <mateusz@nitrokey.com>" )
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
SET(CPACK_DEBIAN_PACKAGE_SECTION "utils")


SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libappindicator1, libnotify4, libqt5widgets5, libqt5gui5, libqt5core5a")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/debian/postinst")


# Rpm
#execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE CPACK_RPM_PACKAGE_ARCHITECTURE)
SET(CPACK_RPM_PACKAGE_DESCRIPTION  "Use and manage your Nitrokey")

SET(CPACK_RPM_PACKAGE_SUMMARY "Use and manage your Nitrokey")
SET(CPACK_RPM_PACKAGE_VENDOR "Nitrokey")
SET(CPACK_RPM_PACKAGE_MAINTAINER "Mateusz Zalega <mateusz@nitrokey.com>" )
SET(CPACK_RPM_PACKAGE_PRIORITY "optional")
SET(CPACK_RPM_PACKAGE_SECTION "utils")

# Prevent RPM directory collisions by excluding those already provided by
# package dependencies. AFAIK this can't be automated.
LIST(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION 
    "/etc/bash_completion.d"
    "/etc/udev"
    "/etc/udev/rules.d"
    "/usr/share/locale"
    "/usr/share/locale/de_DE"
    "/usr/share/locale/de_DE/LC_MESSAGES"
    "/usr/share/icons"
    "/usr/share/icons/hicolor"
    "/usr/share/icons/hicolor/16x16"
    "/usr/share/icons/hicolor/16x16/apps"
    "/usr/share/icons/hicolor/22x22"
    "/usr/share/icons/hicolor/22x22/apps"
    "/usr/share/icons/hicolor/24x24"
    "/usr/share/icons/hicolor/24x24/apps"
    "/usr/share/icons/hicolor/48x48"
    "/usr/share/icons/hicolor/48x48/apps"
    "/usr/share/icons/hicolor/32x32"
    "/usr/share/icons/hicolor/32x32/apps"
    "/usr/share/icons/hicolor/128x128"
    "/usr/share/icons/hicolor/128x128/apps"
    "/usr/share/icons/hicolor/scalable"
    "/usr/share/icons/hicolor/scalable/apps"
    "/usr/share/icons/ubuntu-mono-dark"
    "/usr/share/icons/ubuntu-mono-dark/apps"
    "/usr/share/icons/ubuntu-mono-dark/apps/22"
    "/usr/share/icons/ubuntu-mono-dark/apps/24"
    "/usr/share/icons/ubuntu-mono-dark/apps/48"
    "/usr/share/icons/ubuntu-mono-dark/apps/16"
    "/usr/share/icons/ubuntu-mono-light"
    "/usr/share/icons/ubuntu-mono-light/apps"
    "/usr/share/icons/ubuntu-mono-light/apps/22"
    "/usr/share/icons/ubuntu-mono-light/apps/24"
    "/usr/share/icons/ubuntu-mono-light/apps/48"
    "/usr/share/icons/ubuntu-mono-light/apps/16"
    "/usr/share/applications"
    "/usr/share/pixmaps"
)

INCLUDE(CPack)
