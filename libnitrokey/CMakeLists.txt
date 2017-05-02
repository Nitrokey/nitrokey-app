cmake_minimum_required(VERSION 3.1)
IF (UNIX)
    OPTION(USE_CLANG "Use CLang" FALSE)
    IF(USE_CLANG)
        set(CMAKE_CXX_COMPILER "/usr/bin/clang++" CACHE string "clang++ compiler" FORCE)
    ELSE()
        set(CMAKE_CXX_COMPILER)
    ENDIF()
    OPTION(ADD_ASAN "Use ASAN to show memory issues" FALSE)
    OPTION(ADD_TSAN "Use TSAN to show thread issues" FALSE)
    IF(ADD_ASAN)
        SET(EXTRA_LIBS ${EXTRA_LIBS} asan )
        ADD_DEFINITIONS(-fsanitize=address -fno-omit-frame-pointer)
    ENDIF()
    IF(ADD_TSAN)
        SET(EXTRA_LIBS ${EXTRA_LIBS} tsan )
        SET(USE_CLANG TRUE)
        ADD_DEFINITIONS(-fsanitize=thread -fno-omit-frame-pointer -fPIC -g) #use with clang
    ENDIF()
    IF(ADD_TSAN AND ADD_ASAN)
        message(FATAL_ERROR "TSAN and ASAN cannot be used at the same time")
    ENDIF()
ENDIF()

project(libnitrokey)
SET(PROJECT_VERSION "3.0-alpha")
set(CMAKE_CXX_STANDARD 14)

OPTION(LIBNITROKEY_STATIC "Build libnitrokey statically" TRUE)


OPTION(COMPILE_TESTS "Compile tests" FALSE)
IF (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
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

IF(APPLE)
	include_directories(hidapi/hidapi)
	add_library(hidapi-libusb STATIC hidapi/mac/hid.c )
	target_link_libraries(hidapi-libusb "-framework CoreFoundation" "-framework IOKit")
ELSEIF(UNIX)
#	add_library(hidapi-libusb STATIC hidapi/libusb/hid.c )
ELSEIF(WIN32)
	include_directories(hidapi/hidapi)
	add_library(hidapi-libusb STATIC hidapi/windows/hid.c )
	target_link_libraries(hidapi-libusb setupapi)
ENDIF()


IF (NOT LIBNITROKEY_STATIC)
    add_library(nitrokey SHARED ${SOURCE_FILES})
    add_library(nitrokey-log SHARED ${SOURCE_FILES})
    install (TARGETS nitrokey DESTINATION "lib")
	SET(LIBNAME nitrokey)
ELSE()
    add_library(nitrokey-static STATIC ${SOURCE_FILES})    
    add_library(nitrokey-static-log STATIC ${SOURCE_FILES})
	SET(LIBNAME nitrokey-static)
ENDIF()


target_link_libraries(${LIBNAME} hidapi-libusb)
target_link_libraries(${LIBNAME}-log hidapi-libusb)

OPTION(ERROR_ON_WARNING "Stop compilation on warning found (not supported for MSVC)" ON)
if (NOT MSVC)
    set(COMPILE_FLAGS "-Wall -Wno-unused-function -Wcast-qual -Woverloaded-virtual")
    IF(NOT APPLE)
        if (ERROR_ON_WARNING)
            set(COMPILE_FLAGS "${COMPILE_FLAGS} -Werror")
        endif()
    ENDIF()
    SET_TARGET_PROPERTIES(${LIBNAME} ${LIBNAME}-log PROPERTIES COMPILE_FLAGS ${COMPILE_FLAGS} )
endif()

SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES COMPILE_DEFINITIONS "NO_LOG")


file(GLOB LIB_INCLUDES "include/libnitrokey/*.h")
install (FILES ${LIB_INCLUDES} DESTINATION "include")

IF (COMPILE_TESTS)
    include_directories(unittest/Catch/include)

    add_library(catch SHARED unittest/catch_main.cpp )

    add_executable (test_C_API unittest/test_C_API.cpp)
    target_link_libraries (test_C_API ${EXTRA_LIBS} ${LIBNAME} catch)

    add_executable (test2 unittest/test2.cc)
    target_link_libraries (test2 ${EXTRA_LIBS} ${LIBNAME} catch)

    add_executable (test3 unittest/test3.cc)
    target_link_libraries (test3 ${EXTRA_LIBS} ${LIBNAME} catch)

    add_executable (test_HOTP unittest/test_HOTP.cc)
    target_link_libraries (test_HOTP ${EXTRA_LIBS} ${LIBNAME} catch)

    add_executable (test1 unittest/test.cc)
    target_link_libraries (test1 ${EXTRA_LIBS} ${LIBNAME} catch)

    #run with 'make test' or 'ctest'
    #needs connected PRO device for success
    #warning: it may delete data on the device
    include (CTest)
    add_test (runs test_C_API)
ENDIF()




#SET(CPACK_GENERATOR
#        "DEB;RPM")
# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
        "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set (CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
include (CPack)
