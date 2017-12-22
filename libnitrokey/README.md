[![Stories in Ready](https://badge.waffle.io/Nitrokey/libnitrokey.png?label=ready&title=Ready)](https://waffle.io/Nitrokey/libnitrokey)
[![Build Status](https://travis-ci.org/Nitrokey/libnitrokey.svg?branch=master)](https://travis-ci.org/Nitrokey/libnitrokey)
# libnitrokey
Libnitrokey is a project to communicate with Nitrokey Pro and Storage devices in a clean and easy manner. Written in C++14, testable with `py.test` and `Catch` frameworks, with C API, Python access (through CFFI and C API, in future with Pybind11).

The development of this project is aimed to make it itself a living documentation of communication protocol between host and the Nitrokey stick devices. The command packets' format is described here: [Pro v0.7](include/stick10_commands.h), [Pro v0.8](include/stick10_commands_0.8.h), [Storage](include/stick20_commands.h). Handling and additional operations are described here: [NitrokeyManager.cc](NitrokeyManager.cc).

A C++14 complying compiler is required due to heavy use of variable templates. For feature support tables please check [table 1](https://gcc.gnu.org/projects/cxx-status.html#cxx14) or [table 2](http://en.cppreference.com/w/cpp/compiler_support).

Libnitrokey is developed and tested with the latest compilers: g++ 6.2, clang 3.8. We use Travis CI to test builds also on g++ 5.4 and under OSX compilers starting up from xcode 6.4 environment (OSX 10.10). 

## Getting sources
This repository uses `git submodules`.
To clone please use git's `--recursive` option like in:
```bash
git clone --recursive https://github.com/Nitrokey/libnitrokey.git
```
or for already cloned repository:
```bash
git clone https://github.com/Nitrokey/libnitrokey.git
cd libnitrokey
git submodule update --init --recursive
```

## Dependencies
Following libraries are needed to use libnitrokey on Linux (names of the packages on Ubuntu):
- libhidapi-dev [(HID API)](http://www.signal11.us/oss/hidapi/)
- libusb-1.0-0-dev 


## Compilation
libnitrokey uses CMake as its main build system. As a secondary option it offers building through Qt's qMake.
### Qt
A .pro project file is provided for direct compilation and for inclusion to other projects.

### Windows MS Visual Studio 2017
Lately Visual Studio has started handling CMake files directly. After opening the project's directory it should recognize it and initialize build system. Afterwards please run:
1. `CMake -> Cache -> View Cache CMakeLists.txt -> CMakeLists.txt` to edit settings
2. `CMake -> Build All` to build

It is possible too to use CMake GUI directly with its settings editor.

### Linux CLI
To compile please run following sequence of commands:
```bash
# assuming current dir is ./libnitrokey/
mkdir -p build
cd build
cmake .. <OPTIONS>
make -j2
```

By default (with empty `<OPTIONS>` string) this will create two static library files - build/libnitrokey-static.a and build/libnitrokey-static-log.a. If you wish to build another version (e.g. shared library to use with Python) you can use as `<OPTIONS>` string `-DLIBNITROKEY_STATIC=OFF`. All options could be listed with `cmake .. -L` or instead `cmake` a `ccmake ..` tool could be used for configuration (where `..` is the path with `CMakeLists.txt` file). `ccmake` shows also description of the build parameters.

If you have trouble compiling or running the library you can check [.travis.yml](.travis.yml) file for configuration details. This file is used by our CI service to make test builds on OSX and Ubuntu 14.04.

Other build options (all take either `ON` or `OFF`):
* ADD_ASAN - add tests for memory leaks and out-of-bounds access
* ADD_TSAN - add tests for threads race, needs USE_CLANG
* USE_CLANG - forces Clang as the compiler
* COMPILE_TESTS - compile C++ tests



# Using libnitrokey with Python
To use libnitrokey with Python a [CFFI](http://cffi.readthedocs.io/en/latest/overview.html) library is required (either 2.7+ or 3.0+). It can be installed with:
```bash
pip install --user cffi # for python 2.x
pip3 install cffi # for python 3.x
```
Just import it, read the C API header and it is done! You have access to the library. Here is an example printing HOTP code for Pro or Storage device, assuming it is run in root directory [(full example)](python_bindings_example.py):
```python
#!/usr/bin/env python
import cffi

ffi = cffi.FFI()
get_string = ffi.string


def get_library():
    fp = 'NK_C_API.h'  # path to C API header

    declarations = []
    with open(fp, 'r') as f:
        declarations = f.readlines()

    a = iter(declarations)
    for declaration in a:
        if declaration.startswith('NK_C_API'):
            declaration = declaration.replace('NK_C_API', '').strip()
            while not ';' in declaration:
                declaration += (next(a)).strip()
            print(declaration)
            ffi.cdef(declaration, override=True)

    C = None
    import os, sys
    path_build = os.path.join(".", "build")
    paths = [ os.path.join(path_build,"libnitrokey-log.so"),
              os.path.join(path_build,"libnitrokey.so")]
    for p in paths:
        print p
        if os.path.exists(p):
            C = ffi.dlopen(p)
            break
        else:
            print("File does not exist: " + p)
            print("Trying another")
    if not C:
        print("No library file found")
        sys.exit(1)

    C.NK_set_debug(False)
    nk_login = C.NK_login_auto() # try to connect firstly to Pro and then to Storage
    if nk_login != 1:
        print('No devices detected!')
    assert nk_login != 0  # returns 0 if not connected or wrong model or 1 when connected
    global device_type
    firmware_version = C.NK_get_major_firmware_version()
    model = 'P' if firmware_version in [7,8] else 'S'
    device_type = (model, firmware_version)

    C.NK_set_debug(True)

    return C


def get_hotp_code(lib, i):
    return lib.NK_get_hotp_code(i)


libnitrokey = get_library()
libnitrokey.NK_set_debug(False)  # do not show debug messages (log library only)

hotp_slot_code = get_hotp_code(libnitrokey, 1)
print('Getting HOTP code from Nitrokey device: ')
print(hotp_slot_code)
libnitrokey.NK_logout()  # disconnect device
```

In case  no devices are connected, a friendly message will be printed.
All available functions for C and Python are listed in [NK_C_API.h](NK_C_API.h). Please check `Documentation` section below.

## Documentation
The documentation of C API is included in the sources (could be  generated with doxygen if requested).
Please check NK_C_API.h (C API) for high level commands and include/NitrokeyManager.h (C++ API). All devices' commands are listed along with packet format in include/stick10_commands.h and include/stick20_commands.h respectively for Nitrokey Pro and Nitrokey Storage products.

# Tests
Warning! Before you run unittests please either change both your Admin and User PINs on your Nitrostick to defaults (`12345678` and `123456` respectively) or change the values in tests source code. If you do not change them the tests might lock your device. If it's too late, you can always reset your Nitrokey using instructions from [homepage](https://www.nitrokey.com/de/documentation/how-reset-nitrokey).

## Python tests
Libnitrokey has a great suite of tests written in Python 3 under the path: `unittest/test_*.py`: 
* `test_pro.py` - contains tests of OTP, Password Safe and PIN control functionality. Could be run on both Pro and Storage devices.
* `test_storage.py` - contains tests of Encrypted Volumes functionality. Could be run only on Storage.
The tests themselves show how to handle common requests to device.
Before running please install all required libraries with:
```bash
cd unittest
pip install --user -r requirements.txt
```
To run them please execute: 
```bash
# substitute <dev> with either pro or storage
py.test -v test_<dev>.py
# more specific use - run tests containing in name <test_name> 5 times:
py.test -v test_<dev>.py -k <test_name> --count 5

```
For additional documentation please check the following for [py.test installation](http://doc.pytest.org/en/latest/getting-started.html). For better coverage [randomly plugin](https://pypi.python.org/pypi/pytest-randomly) is installed - it randomizes the test order allowing to detect unseen dependencies between the tests.

## C++ tests
There are also some unit tests implemented in C++, placed in unittest directory. They are not written as extensively as Python tests and are rather more a C++ low level interface check, often not using C++ API from NitrokeyManager.cc. Some of them are: [test_HOTP.cc](https://github.com/Nitrokey/libnitrokey/blob/master/unittest/test_HOTP.cc)
[test.cc](https://github.com/Nitrokey/libnitrokey/blob/master/unittest/test.cc)
Unit tests were written and tested on Ubuntu 16.04/16.10. To run them just execute binaries built in ./libnitrokey/build dir after enabling them by passing `-DCOMPILE_TESTS` option like in `cmake .. -DCOMPILE_TESTS && make`. 


The documentation of how it works could be found in nitrokey-app project's README on Github:
[Nitrokey-app - internals](https://github.com/Nitrokey/nitrokey-app/blob/master/README.md#internals).

To peek/debug communication with device running nitrokey-app (0.x branch) in debug mode (`-d` switch) and checking the logs
(right click on tray icon and then 'Debug') might be helpful. Latest Nitrokey App (1.x branch) uses libnitrokey to communicate with device. Once linked with `libnitrokey-log` it will print all communication to the console. Additionally crosschecking with
firmware code should show how things works:
[report_protocol.c](https://github.com/Nitrokey/nitrokey-pro-firmware/blob/master/src/keyboard/report_protocol.c)
(for Nitrokey Pro, for Storage similarly).

# Known issues / tasks
* Currently only one device can be connected at a time
* C++ API needs some reorganization to C++ objects (instead of pointers to arrays). This will be also preparing for integration with Pybind11,
* Fix compilation warnings

Other tasks might be listed either in [TODO](TODO) file or on project's issues page.

# License
This project is licensed under LGPL version 3. License text could be found under [LICENSE](LICENSE) file.

# Roadmap
To check what issues will be fixed and when please check [milestones](https://github.com/Nitrokey/libnitrokey/milestones) page.
