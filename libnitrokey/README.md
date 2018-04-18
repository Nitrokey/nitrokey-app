[![Build Status](https://travis-ci.org/Nitrokey/libnitrokey.svg?branch=master)](https://travis-ci.org/Nitrokey/libnitrokey)
[![Waffle.io - Columns and their card count](https://badge.waffle.io/Nitrokey/libnitrokey.svg?columns=ready,in%20progress,test,waiting%20for%20feedback)](https://waffle.io/Nitrokey/libnitrokey)

# libnitrokey
libnitrokey is a project to communicate with Nitrokey Pro and Storage devices in a clean and easy manner. Written in C++14, testable with `py.test` and `Catch` frameworks, with C API, Python access (through CFFI and C API, in future with Pybind11).

The development of this project is aimed to make it itself a living documentation of communication protocol between host and the Nitrokey stick devices. The command packets' format is described here: [Pro v0.7](include/stick10_commands.h), [Pro v0.8](include/stick10_commands_0.8.h), [Storage](include/stick20_commands.h). Handling and additional operations are described here: [NitrokeyManager.cc](NitrokeyManager.cc).

A C++14 complying compiler is required due to heavy use of variable templates. For feature support tables please check [table 1](https://gcc.gnu.org/projects/cxx-status.html#cxx14) or [table 2](http://en.cppreference.com/w/cpp/compiler_support).

libnitrokey is developed and tested with the latest compilers: g++ 6.2, clang 3.8. We use Travis CI to test builds also on g++ 5.4 and under OSX compilers starting up from xcode 8.2 environment. 

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
A Qt's .pro project file is provided for direct compilation and for inclusion to other projects.
Using it directly is not recommended due to lack of dependencies check and not implemented library versioning.
Compilation is tested with Qt 5.6 and greater.

Quick start example:
```bash
mkdir -p build
cd build
qmake ..
make -j2
```

### Windows MS Visual Studio 2017
Lately Visual Studio has started handling CMake files directly. After opening the project's directory it should recognize it and initialize build system. Afterwards please run:
1. `CMake -> Cache -> View Cache CMakeLists.txt -> CMakeLists.txt` to edit settings
2. `CMake -> Build All` to build

It is possible too to use CMake GUI directly with its settings editor.

### CMake
To compile please run following sequence of commands:
```bash
# assuming current dir is ./libnitrokey/
mkdir -p build
cd build
cmake .. <OPTIONS>
make -j2
```

By default (with empty `<OPTIONS>` string) this will create in `build/` directory a shared library (.so, .dll or .dynlib). If you wish to build static version you can use as `<OPTIONS>` string `-DBUILD_SHARED_LIBS=OFF`. 

All options could be listed with `cmake .. -L` or instead `cmake` a `ccmake ..` tool could be used for configuration (where `..` is the path to directory with `CMakeLists.txt` file). `ccmake` shows also description of the build parameters.

If you have trouble compiling or running the library you can check [.travis.yml](.travis.yml) file for configuration details. This file is used by Travis CI service to make test builds on OSX and Ubuntu 14.04.

Other build options (all take either `ON` or `OFF`):
* ADD_ASAN - add tests for memory leaks and out-of-bounds access
* ADD_TSAN - add tests for threads race, needs USE_CLANG
* COMPILE_TESTS - compile C++ tests
* COMPILE_OFFLINE_TESTS - compile C++ tests, that do not require any device to be connected
* LOG_VOLATILE_DATA (default: OFF) - include secrets in log (PWS passwords, PINs etc)
* NO_LOG (default: OFF) - do not compile LOG statements - will make library smaller, but without any diagnostic messages



# Using libnitrokey with Python
To use libnitrokey with Python a [CFFI](http://cffi.readthedocs.io/en/latest/overview.html) library is required (either 2.7+ or 3.0+). It can be installed with:
```bash
pip install --user cffi # for python 2.x
pip3 install cffi # for python 3.x
```
Just import it, read the C API header and it is done! You have access to the library. Here is an example (in Python 2) printing HOTP code for Pro or Storage device, assuming it is run in root directory [(full example)](python_bindings_example.py):
```python
#!/usr/bin/env python2
import cffi

ffi = cffi.FFI()
get_string = ffi.string

def get_library():
    fp = 'NK_C_API.h'  # path to C API header

    declarations = []
    with open(fp, 'r') as f:
        declarations = f.readlines()

    cnt = 0
    a = iter(declarations)
    for declaration in a:
        if declaration.strip().startswith('NK_C_API'):
            declaration = declaration.replace('NK_C_API', '').strip()
            while ';' not in declaration:
                declaration += (next(a)).strip()
            # print(declaration)
            ffi.cdef(declaration, override=True)
            cnt +=1
    print('Imported {} declarations'.format(cnt))


    C = None
    import os, sys
    path_build = os.path.join(".", "build")
    paths = [
            os.environ.get('LIBNK_PATH', None),
            os.path.join(path_build,"libnitrokey.so"),
            os.path.join(path_build,"libnitrokey.dylib"),
            os.path.join(path_build,"libnitrokey.dll"),
            os.path.join(path_build,"nitrokey.dll"),
    ]
    for p in paths:
        if not p: continue
        print("Trying " +p)
        p = os.path.abspath(p)
        if os.path.exists(p):
            print("Found: "+p)
            C = ffi.dlopen(p)
            break
        else:
            print("File does not exist: " + p)
    if not C:
        print("No library file found")
        sys.exit(1)

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
Please check [NK_C_API.h](NK_C_API.h) (C API) for high level commands and [include/NitrokeyManager.h](include/NitrokeyManager.h) (C++ API). All devices' commands are listed along with packet format in [include/stick10_commands.h](include/stick10_commands.h) and [include/stick20_commands.h](include/stick20_commands.h) respectively for Nitrokey Pro and Nitrokey Storage products.

# Tests
Warning! Before you run unittests please either change both your Admin and User PINs on your Nitrostick to defaults (`12345678` and `123456` respectively) or change the values in tests source code. If you do not change them the tests might lock your device and lose your data. If it's too late, you can reset your Nitrokey using instructions from [homepage](https://www.nitrokey.com/de/documentation/how-reset-nitrokey).

## Python tests
libnitrokey has a great suite of tests written in Python 3 under the path: `unittest/test_*.py`: 
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
There are also some unit tests implemented in C++, placed in unittest directory. They are not written as extensively as Python tests and are rather more a C++ low level interface check, often not using C++ API from `NitrokeyManager.cc`. Some of them are: [test_HOTP.cc](https://github.com/Nitrokey/libnitrokey/blob/master/unittest/test_HOTP.cc),
[test.cc](https://github.com/Nitrokey/libnitrokey/blob/master/unittest/test.cc).
Unit tests were written and tested on Ubuntu 16.04/16.10/17.04. To run them just execute binaries built in ./libnitrokey/build dir after enabling them by passing `-DCOMPILE_TESTS=ON` option like in `cmake .. -DCOMPILE_TESTS=ON && make`. 


The documentation of how it works could be found in nitrokey-app project's README on Github:
[Nitrokey-app - internals](https://github.com/Nitrokey/nitrokey-app/blob/master/README.md#internals).

To peek/debug communication with device running nitrokey-app (0.x branch) in debug mode (`-d` switch) and checking the logs
(right click on tray icon and then 'Debug') might be helpful. Latest Nitrokey App (1.x branch) uses libnitrokey to communicate with device. Once run with `--dl 3` (3 or higher; range 0-5) it will print all communication to the console. Additionally crosschecking with
firmware code should show how things works:
[report_protocol.c](https://github.com/Nitrokey/nitrokey-pro-firmware/blob/master/src/keyboard/report_protocol.c)
(for Nitrokey Pro, for Storage similarly).

# Known issues / tasks
* Currently only one device can be connected at a time (experimental work could be found in `wip-multiple_devices` branch),
* C++ API needs some reorganization to C++ objects (instead of pointers to arrays). This will be also preparing for integration with Pybind11,
* Fix compilation warnings.

Other tasks might be listed either in [TODO](TODO) file or on project's issues page.

# License
This project is licensed under LGPL version 3. License text could be found under [LICENSE](LICENSE) file.

# Roadmap
To check what issues will be fixed and when please check [milestones](https://github.com/Nitrokey/libnitrokey/milestones) page.
