# https://cmake.org/pipermail/cmake/2011-May/044166.html
IF(NOT DEFINED CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS)
    SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS ON)
ENDIF()

cmake_minimum_required(VERSION 3.1)
IF (UNIX)
    OPTION(ADD_ASAN "Use ASAN to show memory issues" FALSE)
    OPTION(ADD_TSAN "Use TSAN to show thread issues" FALSE)
    IF(ADD_ASAN)
        SET(EXTRA_LIBS ${EXTRA_LIBS} asan )
        ADD_COMPILE_OPTIONS(-fsanitize=address -fno-omit-frame-pointer)
    ENDIF()
    IF(ADD_TSAN)
        SET(EXTRA_LIBS ${EXTRA_LIBS} tsan )
        SET(USE_CLANG TRUE)
        ADD_COMPILE_OPTIONS(-fsanitize=thread -fno-omit-frame-pointer -fPIC -g) #use with clang
    ENDIF()
    IF(ADD_TSAN AND ADD_ASAN)
        message(FATAL_ERROR "TSAN and ASAN cannot be used at the same time")
    ENDIF()
ENDIF()

project(libnitrokey LANGUAGES C CXX VERSION 3.1.0)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include(GNUInstallDirs)



IF (NOT CMAKE_BUILD_TYPE)
    IF(APPLE)
        # Issues occur when build with enabled optimizations
        set(CMAKE_BUILD_TYPE Debug)
    ELSE()
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
    ENDIF()
ENDIF()
MESSAGE("${PROJECT_NAME}: Build type: ${CMAKE_BUILD_TYPE}")

include_directories(hidapi)
include_directories(include)
set(SOURCE_FILES
    include/command.h
    include/command_id.h
    include/cxx_semantics.h
    include/device.h
    include/device_proto.h
    include/dissect.h
    include/inttypes.h
    include/log.h
    include/misc.h
    include/NitrokeyManager.h
    include/stick10_commands.h
    include/stick20_commands.h
    include/CommandFailedException.h
    include/LibraryException.h
    include/LongOperationInProgressException.h
    include/stick10_commands_0.8.h
    command_id.cc
    device.cc
    log.cc
    misc.cc
    NitrokeyManager.cc
    NK_C_API.h
    NK_C_API.cc
        DeviceCommunicationExceptions.cpp)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build all libraries as shared")
add_library(nitrokey ${SOURCE_FILES})

IF(APPLE)
    include_directories(hidapi/hidapi)
    add_library(hidapi-libusb STATIC hidapi/mac/hid.c )
    target_link_libraries(hidapi-libusb "-framework CoreFoundation" "-framework IOKit")
    target_link_libraries(nitrokey hidapi-libusb)
ELSEIF(UNIX)
#    add_library(hidapi-libusb STATIC hidapi/libusb/hid.c )
    find_package(PkgConfig)
    pkg_search_module(HIDAPI_LIBUSB REQUIRED hidapi-libusb)
    target_compile_options(nitrokey PRIVATE ${HIDAPI_LIBUSB_CFLAGS})
    target_link_libraries(nitrokey ${HIDAPI_LIBUSB_LDFLAGS})
ELSEIF(WIN32)
    include_directories(hidapi/hidapi)
    add_library(hidapi-libusb STATIC hidapi/windows/hid.c )
    target_link_libraries(hidapi-libusb setupapi)
    target_link_libraries(nitrokey hidapi-libusb)
ENDIF()

set_target_properties(nitrokey PROPERTIES
	VERSION ${libnitrokey_VERSION}
	SOVERSION ${libnitrokey_VERSION_MAJOR})

OPTION(ERROR_ON_WARNING "Stop compilation on warning found (not supported for MSVC)" OFF)
if (NOT MSVC)
    set(COMPILE_FLAGS "-Wall -Wno-unused-function -Wcast-qual -Woverloaded-virtual")
    IF(NOT APPLE)
        if (ERROR_ON_WARNING)
            set(COMPILE_FLAGS "${COMPILE_FLAGS} -Werror")
        endif()
    ENDIF()
    SET_TARGET_PROPERTIES(nitrokey PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS} )
endif()

OPTION(NO_LOG "Compile without logging functionality and its strings (decreases size)" OFF)
IF (NO_LOG)
    SET_TARGET_PROPERTIES(nitrokey PROPERTIES COMPILE_DEFINITIONS "NO_LOG")
ENDIF()

OPTION(LOG_VOLATILE_DATA "Log volatile data (debug)" OFF)
IF (LOG_VOLATILE_DATA)
    SET_TARGET_PROPERTIES(nitrokey PROPERTIES COMPILE_DEFINITIONS "LOG_VOLATILE_DATA")
ENDIF()


file(GLOB LIB_INCLUDES "include/*.h")
install (FILES ${LIB_INCLUDES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})
install (TARGETS nitrokey DESTINATION ${CMAKE_INSTALL_LIBDIR})

# configure and install pkg-config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libnitrokey.pc.in ${CMAKE_CURRENT_BINARY_DIR}/libnitrokey-1.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libnitrokey-1.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

OPTION(COMPILE_TESTS "Compile tests" FALSE)
OPTION(COMPILE_OFFLINE_TESTS "Compile offline tests" FALSE)

IF(COMPILE_OFFLINE_TESTS OR COMPILE_TESTS)
    include_directories(unittest/Catch/include)
    add_library(catch STATIC unittest/catch_main.cpp )
ENDIF()

IF(COMPILE_OFFLINE_TESTS)
    add_executable (test_offline unittest/test_offline.cc)
    target_link_libraries (test_offline ${EXTRA_LIBS} nitrokey catch)
    #run with 'make test' or 'ctest'
    include (CTest)
    add_test (runs test_offline)
ENDIF()

IF (COMPILE_TESTS)
    #needs connected PRO device for success
    #warning: it may delete data on the device
    add_executable (test_C_API unittest/test_C_API.cpp)
    target_link_libraries (test_C_API ${EXTRA_LIBS} nitrokey catch)

    add_executable (test2 unittest/test2.cc)
    target_link_libraries (test2 ${EXTRA_LIBS} nitrokey catch)

    add_executable (test3 unittest/test3.cc)
    target_link_libraries (test3 ${EXTRA_LIBS} nitrokey catch)

    add_executable (test_HOTP unittest/test_HOTP.cc)
    target_link_libraries (test_HOTP ${EXTRA_LIBS} nitrokey catch)

    add_executable (test1 unittest/test.cc)
    target_link_libraries (test1 ${EXTRA_LIBS} nitrokey catch)

    add_executable (test_issues unittest/test_issues.cc)
    target_link_libraries (test_issues ${EXTRA_LIBS} nitrokey catch)

ENDIF()



#SET(CPACK_GENERATOR
#        "DEB;RPM")
# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set (CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
include (CPack)
