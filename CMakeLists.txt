CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)
cmake_policy(SET CMP0043 OLD) # cmake --help-policy CMP0043

PROJECT(NitrokeyApp CXX)
SET(PROJECT_VERSION "1.0-alpha")
set(CMAKE_CXX_STANDARD 14)

OPTION(ADD_GIT_INFO "Add information about source code version from Git repository" TRUE)

IF (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
ENDIF()
MESSAGE("${PROJECT_NAME}: Build type: ${CMAKE_BUILD_TYPE}")

IF(UNIX)
    OPTION(ADD_ASAN "Use ASAN to show memory issues" FALSE)
    OPTION(ADD_TSAN "Use TSAN to show thread issues" FALSE)
    IF(ADD_ASAN)
        SET(EXTRA_LIBS ${EXTRA_LIBS} asan )
        ADD_DEFINITIONS(-fsanitize=address -fno-omit-frame-pointer)
    ENDIF()
    IF(ADD_TSAN)
        SET(EXTRA_LIBS ${EXTRA_LIBS} tsan )
        ADD_DEFINITIONS(-fsanitize=thread -fno-omit-frame-pointer  -g) #use with clang
    ENDIF()
ENDIF()


#add CMake build info
ADD_DEFINITIONS(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
ADD_DEFINITIONS(-DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER_ID}, ${CMAKE_CXX_COMPILER}, built on: ${CMAKE_SYSTEM} ")
ADD_DEFINITIONS(-DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}")

add_subdirectory (libnitrokey)

IF(LIBNITROKEY_STATIC)
    SET(NK_LIB nitrokey-static)
ELSE()
    SET(NK_LIB nitrokey)
ENDIF()

# QT configuration
FIND_PACKAGE(Qt5Core)
FIND_PACKAGE(Qt5Gui)
FIND_PACKAGE(Qt5Widgets)


# add git version info
IF(ADD_GIT_INFO)
execute_process(
        COMMAND git describe --abbrev=4 HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
    add_definitions(-DGIT_VERSION="${GIT_VERSION}")
ELSE()
    add_definitions(-DGIT_VERSION="")
ENDIF()

add_definitions(-DGUI_VERSION="${PROJECT_VERSION}")


# for out of source build
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Directory structure

SET(ROOTDIR ${CMAKE_CURRENT_SOURCE_DIR})
SET(UIDIR ${ROOTDIR}/ui)
SET(SRCDIR ${ROOTDIR}/src)
SET(SRCUIDIR ${SRCDIR}/ui)
SET(UTILSDIR ${SRCDIR}/utils)


# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Resource files
SET(qrc_files
    resources.qrc
)

# create translation files to include into resource file
# remember to add new translations files there
ADD_SUBDIRECTORY(i18n)
find_package(Qt5LinguistTools)
file(GLOB i18n_files "i18n/*.ts")
qt5_create_translation(i18n_resources ${FILES_TO_TRANSLATE} ${i18n_files})
qt5_add_translation(TRANSLATION_QM ${i18n_files})

IF (WIN32)
ELSEIF(APPLE)
ELSEIF (${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    SET( platform_specific_sources ${SRCDIR}/systemutils.cpp)
ENDIF () # WIN32

INCLUDE_DIRECTORIES(
  ${UIDIR}
  ${SRCDIR}
  ${UTILSDIR}
  ${SRCUIDIR}
    cppcodec
)

# c/cpp sources
SET(nitrokey_app_sources
    ${SRCUIDIR}/aboutdialog.cpp
    ${SRCUIDIR}/nitrokey-applet.cpp
    ${SRCDIR}/hotpslot.cpp
    ${SRCDIR}/main.cpp
    ${SRCDIR}/core/SecureString.cpp
    ${SRCUIDIR}/mainwindow.cpp
    ${SRCUIDIR}/pindialog.cpp
#    ${SRCUIDIR}/securitydialog.cpp
    ${SRCUIDIR}/stick20changepassworddialog.cpp
    ${SRCUIDIR}/stick20debugdialog.cpp
    ${SRCUIDIR}/stick20hiddenvolumedialog.cpp
    ${SRCUIDIR}/stick20lockfirmwaredialog.cpp
    ${SRCUIDIR}/stick20responsedialog.cpp
    ${SRCUIDIR}/stick20updatedialog.cpp
    ${platform_specific_sources}
        src/libada.cpp src/libada.h src/utils/bool_values.h src/GUI/Tray.cpp src/GUI/Tray.h src/GUI/Clipboard.cpp src/GUI/Clipboard.h src/GUI/Authentication.cpp src/GUI/Authentication.h src/core/ScopedGuard.h src/version.h src/GUI/StorageActions.cpp src/GUI/StorageActions.h
#        src/GUI/PasswordSafe.cpp src/GUI/PasswordSafe.h
        src/core/ThreadWorker.cpp src/core/ThreadWorker.h)

#INSTALL

# Freedesktop files
IF(NOT WIN32)
  install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/data/icons/
    DESTINATION share/icons
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
   DESTINATION lib/udev/rules.d
  )

  # Install autocompletion scripts
  install(FILES
   ${CMAKE_SOURCE_DIR}/data/bash-autocomplete/nitrokey-app
    DESTINATION /etc/bash_completion.d
  )

  install(FILES
    ${CMAKE_SOURCE_DIR}/images/info.png
    ${CMAKE_SOURCE_DIR}/images/quit.png
    ${CMAKE_SOURCE_DIR}/images/safe_zahlenkreis.png
    ${CMAKE_SOURCE_DIR}/images/settings.png
    DESTINATION share/nitrokey
  )

ENDIF () # NOT WIN32


ADD_EXECUTABLE(nitrokey-app ${GUI_TYPE}
  ${nitrokey_app_sources}
  ${i18n_resources}
        ${qrc_files}
)

ADD_EXECUTABLE(nitrokey-app.debug ${GUI_TYPE}
  ${nitrokey_app_sources}
  ${i18n_resources}
        ${qrc_files}
)

INSTALL(TARGETS nitrokey-app nitrokey-app.debug DESTINATION bin)

TARGET_LINK_LIBRARIES(nitrokey-app
  ${EXTRA_LIBS}
  ${NK_LIB}
#  ${platform_specific_libs}
  Qt5::Widgets Qt5::Core Qt5::Gui
)

TARGET_LINK_LIBRARIES(nitrokey-app.debug
  ${EXTRA_LIBS}
  ${NK_LIB}-log
#  ${platform_specific_libs}
  Qt5::Widgets Qt5::Core Qt5::Gui
)





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

SET(PACKAGE_MAINTAINER "Szczepan Zalega <szczepan@nitrokey.com>" )
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER ${PACKAGE_MAINTAINER} )
SET(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
SET(CPACK_DEBIAN_PACKAGE_SECTION "utils")


SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5widgets5, libqt5gui5, libqt5core5a, libhidapi-libusb0")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/debian/postinst")


# Rpm
#execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE CPACK_RPM_PACKAGE_ARCHITECTURE)
SET(CPACK_RPM_PACKAGE_DESCRIPTION  "Use and manage your Nitrokey")

SET(CPACK_RPM_PACKAGE_SUMMARY "Use and manage your Nitrokey")
SET(CPACK_RPM_PACKAGE_VENDOR "Nitrokey")
SET(CPACK_RPM_PACKAGE_MAINTAINER ${PACKAGE_MAINTAINER} )
SET(CPACK_RPM_PACKAGE_PRIORITY "optional")
SET(CPACK_RPM_PACKAGE_SECTION "utils")

# Prevent RPM directory collisions by excluding those already provided by
# package dependencies. AFAIK this can't be automated.
LIST(APPEND CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION 
    "/etc/bash_completion.d"
    "/etc/udev"
    "/etc/udev/rules.d"
    "/usr/lib/udev"
    "/usr/lib/udev/rules.d"
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
