Nitrokey App [![Build Status](https://travis-ci.org/Nitrokey/nitrokey-app.png?branch=master)](https://travis-ci.org/Nitrokey/nitrokey-app)  [![Code Health](https://landscape.io/github/Nitrokey/nitrokey-app/master/landscape.svg?style=flat)](https://landscape.io/github/Nitrokey/nitrokey-app/master)  [![Coverity Scan Build](https://scan.coverity.com/projects/4744/badge.svg)](https://scan.coverity.com/projects/4744)
============
Nitrokey App runs under Windows, Linux and Mac OS. It has been created with Qt Creator and Qt 5 and MinGW 4.4.

The implementation is compatible to the Google Authenticator application which can be used for testing purposes. See [google-authenticator](http://google-authenticator.googlecode.com/git/libpam/totp.html)

Using the application under Linux also requires root privileges, or configuration of device privileges in udev (due to USB communication).

To compile the Nitrokey App under Linux install the package libusb-1.0.0-dev and QT Creator. You may need to add to the .pro file:
QMAKE_CXXFLAGS= -I/usr/include/libusb-1.0
QMAKE_CFLAGS= -I/usr/include/libusb-1.0

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
mkdir git
cd git
Git clone https://github.com/Nitrokey/nitrokey-app.git
```

#### QT5
Prerequisite: [Install QT5](http://www.qt.io/download-open-source/#section-2) or `sudo apt-get install qt5-default qtcreator`

Use QT Creator for compilation or perform the following steps:

1. cd to a build directory parallel to the nitrokey-app directory
2. For 64 bit system: 
   $HOME/Qt/5.5/gcc_64/bin/qmake -spec  $HOME/Qt/5.5/gcc_64/mkspecs/linux-g++-64 -o Makefile $HOME/git/nitrokey-app/nitrokey-app-qt5.pro
4. make -j4

#### Using cmake:
General use:
```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\<prefix\> $HOME/git/nitrokey-app/
make -j4
make install
```

Note: You have to set CMAKE_PREFIX_PATH to your corresponding Qt path.
Example: (If QT5.5 (64bit) was installed in your $HOME DIR): 
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

Requirements: fakeroot, debhelper, hardening-wrapper.


#### Cross Compiling with QT5 for Windows on Ubuntu Linux
Based on [this](https://stackoverflow.com/questions/10934683/how-do-i-configure-qt-for-cross-compilation-from-linux-to-windows-target):

1. sudo apt-get install bison cmake flex intltool libtool scons
2. git clone https://github.com/mxe/mxe.git
3. cd mxe && make qt5
4. export PATH=<mxe root>/usr/bin:$PATH
5. Change to build directory parallel to PC CryptoStickGUI directory, e.g. build-CryptoStickGUI-Win32-release
6. <mxe root>/usr/i686-w64-mingw32.static/qt5/bin/qmake -spec <mxe root>/usr/i686-w64-mingw32.static/qt5/mkspecs/win32-g++ -o Makefile ../nitrokey-app/nitrokey-app-qt5.pro
7. make
8. optional: use upx to compress the executable


#### Compiling for MAC OS
1. Use Qt to compile the Nitrokey App
2. Navigate to <build_dir>/CryptoStickGUI.app/Contents
3. Create a .dmg file
   Go to the build directory and use

     `macdeployqt CryptoStickGUI.app/ -dmg`
     
   CryptoStickGUI.dmg file will be created at the same folder. This is the final file for distributing the App on Mac OS
4. Compress the .dmg package:
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

The report protocol is described [here](https://github.com/Nitrokey/nitrokey-pro-firmware/blob/master/src/INC/report_protocol.h) for Pro and [here](https://github.com/Nitrokey/nitrokey-storage-firmware/blob/master/src/OTP/report_protocol.h) for Storage.
The HID reports are set to 64 bytes. The "output report" is what you get from the device. When you send a report (command), the first byte sets the command type, then you have 59 bytes for your data, and the last 4 bytes are the CRC32 of the whole report.

On the GUI side, you can find examples of sending commands and receiving responses [here](https://github.com/Nitrokey/nitrokey-app/blob/master/device.cpp)

First you create an object of class Command, for that you need a number for the command type, a buffer of data you want to send and the length of that buffer. Then you just use Device::sendCommand(Command *cmd), the CRC32 will be calculated automatically.
To get a response, you just create a new object of class Response and do Response::getResponse(Device *device) and then you can analyze contents of the response object, already divided into fields ([see here](https://github.com/Nitrokey/nitrokey-app/blob/master/response.h)).


