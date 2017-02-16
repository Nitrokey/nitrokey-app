Nitrokey App [![Build Status](https://travis-ci.org/Nitrokey/nitrokey-app.png?branch=master)](https://travis-ci.org/Nitrokey/nitrokey-app) [![Coverity Scan Build](https://scan.coverity.com/projects/4744/badge.svg)](https://scan.coverity.com/projects/4744)
============
Nitrokey App runs under Windows, Linux and Mac OS. It has been created with Qt Creator and Qt 5 and MinGW 4.4. Lately developed under Ubuntu 16.04 with Qt5.5.

The implementation is compatible to the Google Authenticator application which can be used for testing purposes. See [google-authenticator](http://google-authenticator.googlecode.com/git/libpam/totp.html)

Using the application under Linux also requires root privileges, or configuration of device privileges in udev (due to USB communication). Udev configuration is installed automatically with application (either with package or after `make install`).

Known issue: tray icon under Debian Jessy
----------------
Under Debian Jessy application's tray icon might be unavailable. There were reports it can be fixed with updating Qt libraries to 5.4.2 version and up. The packages are available in experimental branch. For more details please refer to: https://github.com/Nitrokey/nitrokey-app/issues/86

Installation and downloads
-------------------------
Ready to use packages and install instructions are available on main site in download section: https://www.nitrokey.com/download

Compilation
===========

To compile the Nitrokey App under Linux install the package `libusb-1.0.0-dev` and QT Creator (optionally). In case it would not work out-of-the-box you may need to add to the .pro file:
```
QMAKE_CXXFLAGS= -I/usr/include/libusb-1.0
QMAKE_CFLAGS= -I/usr/include/libusb-1.0
```

Note: In case `libusb-1.0.0-dev` is not available to install please check other name: `libusb-1.0-0-dev` (the difference is the `-` char between zeroes).



Compiling on Ubuntu Linux
-------------------------
Prerequisites for building on Ubuntu:
libusb-1.0.0-dev
libgtk2.0-dev
libappindicator-dev
libnotify-dev

```
sudo apt-get install libusb-1.0.0-dev libgtk2.0-dev libappindicator-dev libnotify-dev
```

#### Getting the Nitrokey Sources

Clone the Nitrokey repository into your $HOME git folder.

```
cd $HOME
mkdir git && cd git
git clone https://github.com/Nitrokey/nitrokey-app.git
```

#### QT5
Prerequisites: Install Qt manually using [download page](http://www.qt.io/download-open-source/#section-2) or through package manager:
```
sudo apt-get install qt5-default
sudo apt-get install qtcreator #for compilation using IDE
```

Use QT Creator (IDE) for compilation or perform the following steps:

- out of source build:
   1. cd to a build directory
   2. For 64 bit system (assuming Qt is installed in `~/Qt` and the app sources are in `~/git/nitrokey-app`): 
      `$HOME/Qt/5.5/gcc_64/bin/qmake -spec  $HOME/Qt/5.5/gcc_64/mkspecs/linux-g++-64 -o Makefile $HOME/git/nitrokey-app/nitrokey-app-qt5.pro`
   3. `make -j4`

- simple build in source directory (assuming Qt being in `$PATH`):
   `qmake && make -j4`

#### Using cmake:
General use:
```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\<prefix\> $HOME/git/nitrokey-app/
make -j4
make install
```

Note: In case you have downloaded Qt manually from web download page you have to set `CMAKE_PREFIX_PATH` to your corresponding Qt path.
Example: (If QT5.5 (64bit) was installed in your `$HOME` DIR): 
```
export CMAKE_PREFIX_PATH=$HOME/Qt/5.5/gcc_64/
```
Out of the source build is highly recomended, because you are not touching your git'ed source directory. Eg:
```
mkdir build
mkdir install
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../install $HOME/git/nitrokey-app/
make -j4
sudo make install
```

#### Using qmake:
```
qmake
make -j4
make install
```

You can use out-of-the-source build again:
```
mkdir build
cd build
qmake ..
make -j4 && make install
```

#### Building Debian Packages

Execute the following in directory "nitrokey-app":

```
 fakeroot make -f debian/rules binary
```

Cleanup with:
 ```
 fakeroot make -f debian/rules clean
 ```

Requirements: fakeroot, debhelper, hardening-wrapper, qt5-default, gtk2.0, libusb-1.0-0-dev, libappindicator-dev, libnotify-dev.

#### Building RPM and Debian Packages (alternative)
CMake can generate RPM packages using CPack. It can also generate `.deb` package using other method than presented in previous section. To create both packages please execute the following in directory "nitrokey-app":
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4 package
```
This will result in two packages: `.deb` and `.rpm`. 

#### Cross Compiling with QT5 for Windows on Ubuntu Linux (using MXE)

new:

```
# MXE gcc6, x32:
#remove MXE_PLUGIN_DIRS for GCC 5.4.0
git clone https://github.com/mxe/mxe.git && pushd mxe
make MXE_TARGETS=i686-w64-mingw32.static.posix MXE_PLUGIN_DIRS=plugins/gcc6  qtbase # takes about 1 hour first time
popd

#should download nitrokey-app and libnitrokey and hidapi (own clone with applied windows patch)
git clone https://github.com/szszszsz/nitrokey-app --recursive --init
pushd nitrokey-app/libnitrokey/build
ln -s ../../../mxe/usr/bin/
./bin/i686-w64-mingw32.static.posix-cmake ..
make -j
popd

mkdir nitrokey-app/build/
pushd nitrokey-app/build/
ln -s ../mxe/usr/bin/
./bin/i686-w64-mingw32.static.posix-qmake ..
PATH=$PATH:./bin make -j
popd
```


old (for reference):
Based on [this](https://stackoverflow.com/questions/10934683/how-do-i-configure-qt-for-cross-compilation-from-linux-to-windows-target):
1. `sudo apt-get install bison cmake flex intltool libtool scons`
2. `git clone https://github.com/mxe/mxe.git`
3. `cd mxe && make qt5`
4. `export PATH=<mxe root>/usr/bin:$PATH`
5. Change to build directory parallel to source directory, e.g. `build-nitrokeyapp-Win32-release`
6. `<mxe root>/usr/i686-w64-mingw32.static/qt5/bin/qmake -spec <mxe root>/usr/i686-w64-mingw32.static/qt5/mkspecs/win32-g++ -o Makefile ../nitrokey-app/nitrokey-app-qt5.pro`
7. `make`
8. optional: use upx to compress the executable


#### Compiling for MAC OS
1. Use Qt to compile the Nitrokey App
2. Navigate to `<build_dir>/<app_name>/Contents`
3. Create a .dmg file: go to the build directory and use
     
     `macdeployqt <app_name>/ -dmg`
     
   `<app_name>.dmg` file will be created at the same folder. This is the final file for distributing the App on Mac OS
4. Compress the `.dmg` package:
   * Open Disk Utility
   * Select the dmg package from left column (or drag'n'drop)
   * Select Convert, check "compressed" option and then "Save"

Tray
---

Note that the Nitrokey App's graphical interface is based on a QT system tray widget. If you are using a Linux desktop environment that does not support system tray widgets, then you may be unable to access the graphical interface.


Internals
---------
All configuration data including OTP secrets are stored in clear text in the flash of Nitrokey's microcontroller. This is not tamper resistant and may only be used for low to medium security requirements.

By default the OTP serial number is OpenPGP Card's serial number. It can be changed within the GUI. The USB device serial number is set to the card's serial number when the device powers up.

Keyboard Layout: The user will input the token ID values as he wants them displayed, then the gui will translate them to keycodes of the selected layout. The keycodes will be stored on the device, along with a number saying which layout was used, this number is important when the GUI application reads the conifg back from the device (to translate the keycodes back into characters).

The report protocol is described [here](https://github.com/Nitrokey/nitrokey-pro-firmware/blob/master/src/inc/report_protocol.h) for Pro and [here](https://github.com/Nitrokey/nitrokey-storage-firmware/blob/master/src/OTP/report_protocol.h) for Storage.
The HID reports are set to 64 bytes. The "output report" is what you get from the device. When you send a report (command), the first byte sets the command type, then you have 59 bytes for your data, and the last 4 bytes are the CRC32 of the whole report.

On the GUI side, you can find examples of sending commands and receiving responses [here](https://github.com/Nitrokey/nitrokey-app/blob/master/src/device.cpp)

First you create an object of class Command, for that you need a number for the command type, a buffer of data you want to send and the length of that buffer. Then you just use Device::sendCommand(Command *cmd), the CRC32 will be calculated automatically.
To get a response, you just create a new object of class Response and do Response::getResponse(Device *device) and then you can analyze contents of the response object, already divided into fields ([see here](https://github.com/Nitrokey/nitrokey-app/blob/master/src/response.h)).


