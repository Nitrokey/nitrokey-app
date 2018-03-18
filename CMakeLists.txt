CMAKE_MINIMUM_REQUIRED(VERSION 3.1.0 FATAL_ERROR)
cmake_policy(SET CMP0043 OLD) # cmake --help-policy CMP0043

PROJECT(NitrokeyApp LANGUAGES CXX)
SET(PROJECT_VERSION "1.3-beta.1")

include(GNUInstallDirs)


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

OPTION(ADD_GIT_INFO "Add information about source code version from Git repository" TRUE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build all libraries as shared")

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


# QT configuration
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Svg REQUIRED)


# add git version info
IF(ADD_GIT_INFO)
execute_process(
        COMMAND git describe --always --abbrev=4 HEAD
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
qt5_add_resources(RESOURCE_ADDED ${qrc_files})

# create translation files to include into resource file
# remember to add new translations files there
IF (UNIX AND NOT APPLE)
	ADD_SUBDIRECTORY(i18n)
	find_package(Qt5LinguistTools)
	file(GLOB i18n_files "i18n/*.ts")
	qt5_create_translation(i18n_resources ${SRCUIDIR} ${i18n_files})
	qt5_add_translation(TRANSLATION_QM ${i18n_files})
ENDIF()

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
        src/ui/licensedialog.cpp
        src/GUI/ManageWindow.cpp
	src/GUI/graphicstools.cpp
        #        src/GUI/PasswordSafe.cpp src/GUI/PasswordSafe.h
        src/core/ThreadWorker.cpp src/core/ThreadWorker.h)

#INSTALL

# Freedesktop files
IF(NOT WIN32)
  install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/data/icons/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons
  )

  install(FILES
    ${CMAKE_SOURCE_DIR}/data/nitrokey-app.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
  )

  install(FILES
    ${CMAKE_SOURCE_DIR}/data/com.nitrokey.nitrokey-app.appdata.xml
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/metainfo/
  )

  install(FILES
    ${CMAKE_SOURCE_DIR}/data/icons/hicolor/128x128/apps/nitrokey-app.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps
  )

  find_package(PkgConfig)

  # Install Nitrokey udev rules
  # Removed - should be provided by libnitrokey

  # Install autocompletion scripts
  set(PKG_GET_BASH_COMPLETION ${PKG_CONFIG_EXECUTABLE} --variable=completionsdir bash-completion)
  execute_process(COMMAND ${PKG_GET_BASH_COMPLETION} RESULT_VARIABLE ERR OUTPUT_VARIABLE BASH_COMPLETION_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  IF(${ERR})
    set(BASH_COMPLETION_DIR "etc/bash_completion.d")
  ENDIF()
  string(REGEX REPLACE "^/" "" BASH_COMPLETION_DIR "${BASH_COMPLETION_DIR}")
  string(REGEX REPLACE "^usr/" "" BASH_COMPLETION_DIR "${BASH_COMPLETION_DIR}") # usual prefix is usr/local
  message(STATUS "Setting bash-completion dir to ${BASH_COMPLETION_DIR}")

  install(FILES
   ${CMAKE_SOURCE_DIR}/data/bash-autocomplete/nitrokey-app
    DESTINATION ${BASH_COMPLETION_DIR}
  )

ENDIF () # NOT WIN32


ADD_EXECUTABLE(nitrokey-app ${GUI_TYPE}
  ${nitrokey_app_sources}
  ${i18n_resources}
        ${qrc_files}
)

OPTION(ERROR_ON_WARNING "Stop compilation on warning found (not supported for MSVC)" OFF)
if (NOT MSVC)
    set(COMPILE_FLAGS "-Wall -Wno-unused-function -Wcast-qual -Woverloaded-virtual")
    #    IF(NOT APPLE)
    if (ERROR_ON_WARNING)
        set(COMPILE_FLAGS "${COMPILE_FLAGS} -Werror")
    endif()
    SET_TARGET_PROPERTIES(nitrokey-app PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS} )
    #    endif()
endif()

INSTALL(TARGETS nitrokey-app DESTINATION ${CMAKE_INSTALL_BINDIR})

TARGET_LINK_LIBRARIES(nitrokey-app
  ${EXTRA_LIBS}
#  ${platform_specific_libs}
Qt5::Widgets Qt5::Core Qt5::Gui Qt5::Svg
)

# first try to find libnitrokey via pkg-config
find_package(PkgConfig)
pkg_search_module(LIBNITROKEY libnitrokey-1)

if(LIBNITROKEY_FOUND)
  MESSAGE(STATUS "Found system libnitrokey (Cflags: '${LIBNITROKEY_CFLAGS}' Libs: '${LIBNITROKEY_LDFLAGS}')")
  if (BUILD_SHARED_LIBS)
    target_compile_options(nitrokey-app PRIVATE ${LIBNITROKEY_CFLAGS})
    target_link_libraries(nitrokey-app ${LIBNITROKEY_LDFLAGS})
  else()
    target_compile_options(nitrokey-app PRIVATE ${LIBNITROKEY_STATIC_CFLAGS})
    target_link_libraries(nitrokey-app ${LIBNITROKEY_STATIC_LDFLAGS})
  endif()
else()
  MESSAGE("Using bundled libnitrokey")
  add_subdirectory (libnitrokey)
  include_directories(libnitrokey)
  target_link_libraries(nitrokey-app nitrokey)
endif()

# ... same for cppcodec
pkg_search_module(CPPCODEC cppcodec-1)

if(CPPCODEC_FOUND)
  MESSAGE(STATUS "Found system cppcodec (Cflags: '${CPPCODEC_CFLAGS}' Libs: '${CPPCODEC_LDFLAGS}')")
  if (BUILD_SHARED_LIBS)
    target_compile_options(nitrokey-app PRIVATE ${CPPCODEC_CFLAGS})
    target_link_libraries(nitrokey-app ${CPPCODEC_LDFLAGS})
  else()
    target_compile_options(nitrokey-app PRIVATE ${CPPCODEC_STATIC_CFLAGS})
    target_link_libraries(nitrokey-app ${CPPCODEC_STATIC_LDFLAGS})
  endif()
else()
  MESSAGE("Using bundled cppcodec")
  include_directories(3rdparty/cppcodec)
endif()

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
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5widgets5, libqt5gui5, libqt5core5a, libqt5svg5, libhidapi-libusb0")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/debian/postinst")


# Rpm
#execute_process(COMMAND dpkg --print-architecture OUTPUT_VARIABLE CPACK_RPM_PACKAGE_ARCHITECTURE)
SET(CPACK_RPM_PACKAGE_DESCRIPTION  "Use and manage your Nitrokey")
SET(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/GPLv3")
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
