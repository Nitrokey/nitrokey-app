Nitrokey App [![Build Status](https://travis-ci.org/Nitrokey/nitrokey-app.png?branch=master)](https://travis-ci.org/Nitrokey/nitrokey-app) [![Coverity Scan Build](https://scan.coverity.com/projects/4744/badge.svg)](https://scan.coverity.com/projects/4744)
============
Nitrokey App is a cross-platform (runs under Windows, Linux and Mac OS) application created to manage [Nitrokey devices](https://www.nitrokey.com/). Lately developed under Ubuntu 17.10/18.04 with Qt5.9. 
Underneath it uses [libnitrokey](https://github.com/Nitrokey/libnitrokey) to communicate with the supported devices. Both Nitrokey App and libnitrokey are available under GPLv3 license. 

##### Supported devices
Currently Nitrokey App supports: 
- Nitrokey Pro v0.7/v0.8, 
- Nitrokey Storage.
 
For Nitrokey HSM please see [Nitrokey Encryption Tool](https://github.com/Nitrokey/nitrokey-encryption-tool). Nitrokey Start has its own tools in its [firmware repository](https://github.com/Nitrokey/nitrokey-start-firmware/tree/gnuk1.2-regnual-fix).


##### Compatibility
The implementation is compatible to the Google Authenticator application which can be used for testing purposes. Test vectors from proper specifications also works:
- HOTP: An HMAC-Based One-Time Password Algorithm - [RFC4226](https://tools.ietf.org/html/rfc4226),
- TOTP: Time-Based One-Time Password Algorithm - [RFC6238](https://tools.ietf.org/html/rfc6238).

Each libnitrokey's and supported devices' firmware release OTPs are tested automatically, according to the RFCs test vectors. 

##### Builds
Ready to use binaries are available at [releases page](https://github.com/Nitrokey/nitrokey-app/releases). More details could be found at [main download site](https://www.nitrokey.com/download).

##### Using under Linux 
Using the application under Linux requires configuration of device privileges in udev (due to USB communication). 
The configuration is installed automatically with the libnitrokey library (either with a package or after `make install`). Without it application cannot communicate unless run with root privileges.

## Known issues

#### Tray icon under Gnome 3.26

Gnome 3.26 (and later) removed support for the tray dock and tray icon (Ubuntu 18.04 is not affected - it contains own plugin to support it). For more details please see: [NitrokeyApp#274](https://github.com/Nitrokey/nitrokey-app/issues/274). Nitrokey App v1.3 should solve this by introducing main window, which is shown right on application's start. 

#### Tray icon under Debian Jessie
Under Debian Jessie application's tray icon might be unavailable. There were reports it can be fixed with updating Qt libraries to 5.4.2 version and up. The packages are available in experimental OS branch. For more details please refer to: [NitrokeyApp#86](https://github.com/Nitrokey/nitrokey-app/issues/86). Another way to workaround this is using an AppImage binary release, introduced in [Nitrokey App v1.3](https://github.com/Nitrokey/nitrokey-app/releases/tag/v1.3).

Installation and downloads
-------------------------
Ready to use packages and install instructions are available on main site in download section: https://www.nitrokey.com/download

## Compilation
Ready to use Docker containers for building AppImage, Windows and Snapcraft binaries are available at [nitrokey-app.build](https://github.com/Nitrokey/nitrokey-app.build/tree/master/docker) project. There are set [automatic builds using Travis CI](https://travis-ci.org/Nitrokey/nitrokey-app) as well.
Below are detailed instructions how to make a build manually.

### Compiling on Ubuntu Linux

Prerequisites for building on Ubuntu 17.10:
- `build-essential` - for building applications
- `cmake` - for compiling libnitrokey
- `qt5-default` - QT5 library
- `qttools5-dev-tools` - QT5 library tools - generating translations
- `libqt5svg5-dev` - QT5 library for rendering SVG
- `libqt5concurrent5` - QT5 library for concurrent calls
- `pkg-config` - system libraries detection
- `libnitrokey` v3.3 - compiled only, if not already installed in the OS
	- `libusb-1.0-0-dev` - library to communicate with USB devices
	- `libhidapi-dev` - to communicate using HID layer

Whole command for Ubuntu:
```
sudo apt-get install libusb-1.0.0-dev cmake qt5-default qttools5-dev-tools pkg-config libhidapi-dev build-essential libqt5svg5-dev libqt5concurrent5
```

#### Getting the Nitrokey Sources

Clone the Nitrokey-App repository using `git` and `--recursive` switch:
```
git clone https://github.com/Nitrokey/nitrokey-app.git --recursive
```

#### Qt Creator
Prerequisites: Install Qt manually using [download page](http://www.qt.io/download-open-source/#section-2) or through package manager:
```
sudo apt-get install qt5-default
sudo apt-get install qtcreator #for compilation using IDE
```

Please open `nitrokey-app-qt5.pro` file and select `Build All` from `Build` menu.

#### Using CMake:
General use:
```
# in nitrokey-app directory:
mkdir build
cd build
mkdir install
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=./install ..
make -j4
make install
```
This will build and install binary to `./build/install` directory. Please omit `-DCMAKE_INSTALL_PREFIX=./install` in case you would like to install it to the system.

Note: In case you have downloaded Qt manually from web download page you have to set `CMAKE_PREFIX_PATH` to your corresponding Qt path.
Example: (If QT5.5 (64bit) was installed in your `$HOME` DIR): 
```
export CMAKE_PREFIX_PATH=$HOME/Qt/5.5/gcc_64/
```

#### Using QMake:
Please run following commands for out-of-the-source build:
```
mkdir build && cd build
qmake ..
make -j4 
# make install
```


### Debian Packages
#### Building Debian Packages

Execute the following in directory "nitrokey-app":

```
 fakeroot make -f debian/rules binary
```

Cleanup with:
 ```
 fakeroot make -f debian/rules clean
 ```

Requirements: fakeroot, debhelper, hardening-wrapper, qt5-default, libusb-1.0-0-dev, cmake. 

#### Building RPM and Debian Packages (alternative)
CMake can generate RPM packages using CPack. It can also generate `.deb` package using other method than presented in previous section. To create both packages please execute the following in directory "nitrokey-app":
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4 package
```
This will result in two packages: `.deb` and `.rpm`. 

#### Cross Compiling with QT5 for Windows on Ubuntu Linux (using MXE)

Please run following commands:

```
# install dependencies for compilation
sudo apt-get install bison cmake flex intltool libtool scons
# MXE GCC6, x32
#remove MXE_PLUGIN_DIRS switch to use GCC 5.4.0 instead of GCC 6
git clone https://github.com/mxe/mxe.git && pushd mxe
make MXE_TARGETS=i686-w64-mingw32.static.posix MXE_PLUGIN_DIRS=plugins/gcc6  qtbase # takes about 1 hour first time
popd

#following should download nitrokey-app and libnitrokey with hidapi (own clone with applied OS-specific patches)
git clone https://github.com/Nitrokey/nitrokey-app.git --recursive 
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


#### Compiling and creating a package for MAC OS

1. Use Qt (qmake) to compile the Nitrokey App
2. Navigate to `<build_dir>/<app_name>/Contents`
3. Create a .dmg file: go to the build directory and run:    
`macdeployqt <app_name>/ -dmg`. `<app_name>.dmg` file will be created at the same folder. This is the final file for distributing the App on Mac OS
4. Compress the `.dmg` package:
   * Open Disk Utility
   * Select the dmg package from left column (or drag'n'drop)
   * Select Convert, check "compressed" option and then "Save"

## Tray
Note that the Nitrokey App's graphical interface is based on a QT system tray widget. If you are using a Linux desktop environment that does not support system tray widgets, then you may be unable to access the graphical interface. Nitrokey App v1.3 shows main window right on start, offering tray's menu, and it is possible to configure it to quit once the main window is closed.


## Internals
All configuration data including OTP secrets are stored in clear text in the flash of Nitrokey's microcontroller. This is not tamper resistant and may only be used for low to medium security requirements.
Password Safe is encrypted using 256 bit AES key to which access is protected with SIM card.

By default the OTP serial number (OTP Token) is OpenPGP Card's serial number. It can be changed within the GUI for each entry. The USB device serial number is set to the card's serial number when the device powers up.

(disabled feature) Keyboard Layout: The user will input the token ID values as he wants them displayed, then the gui will translate them to keycodes of the selected layout. The keycodes will be stored on the device, along with a number saying which layout was used, this number is important when the GUI application reads the conifg back from the device (to translate the keycodes back into characters).

The report protocol is described [here](https://github.com/Nitrokey/nitrokey-pro-firmware/blob/master/src/inc/report_protocol.h) for Pro and [here](https://github.com/Nitrokey/nitrokey-storage-firmware/blob/master/src/OTP/report_protocol.h) for Storage.
The HID reports are set to 64 bytes. The "output report" is what you get from the device. When you send a report (command), the first byte sets the command type, then you have 59 bytes for your data, and the last 4 bytes are the CRC32 of the whole report.

On the client side, please check documentation of [libnitrokey](https://github.com/Nitrokey/libnitrokey) project.
