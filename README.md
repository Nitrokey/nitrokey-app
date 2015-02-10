Nitrokey App
============
Nitrokey App runs under Windows, Linux and Mac OS. It has been created with Qt Creator 2.4.1 with Qt 4.7.4 and MinGW 4.4.

The implementation is compatible to the Google Authenticator application which can be used for testing purposes. See [google-authenticator](http://google-authenticator.googlecode.com/git/libpam/totp.html)

Using the application under Linux also requires root privileges, or configuration of device privileges in udev (due to USB communication).

To compile hte Nitrokey App under Linux install the package libusb-1.0.0-dev and QT Creator. You may need to add to the .pro file:
QMAKE_CXXFLAGS= -I/usr/include/libusb-1.0
QMAKE_CFLAGS= -I/usr/include/libusb-1.0

Compiling on Ubuntu Linux
-------------------------
#### QT5
Prerequisite: [Install QT5](https://qt-project.org/wiki/Install_Qt_5_on_Ubuntu)

Use QT Creator for compilation or perform the following steps:

1. Change to build directory parallel to PC CryptoStickGUI directory
2. For 64 bit system: 
   ~/bin/qt/5.3/gcc_64/bin/qmake -spec  ~/bin/qt/5.3/gcc_64/mkspecs/linux-g++-64 -o Makefile ../cryptostick.utility/CryptoStickGUI_QT5.pro
3. make

#### QT 4
Prerequisite: sudo apt-get install qt4-default libqt4-dev libusb-1.0.0-dev qtcreator
Use QT Creator for compilation or perform the following steps:

1. Change to build directory parallel to PC CryptoStickGUI directory
2. Configure:
  * For 64 bit system: 
  /usr/bin/qmake-qt4 -spec /usr/share/qt4/mkspecs/linux-g++-64 -o Makefile ../CryptoStickGUI/CryptoStickGUI.pro
  * For 32 bit system: /usr/bin/qmake-qt4 -spec /usr/share/qt4/mkspecs/linux-g++-32 -o Makefile ../CryptoStickGUI/CryptoStickGUI.pro
3. make


#### Using cmake:
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=\<prefix\> \<path to this project's root dir\>
make
make install

Note: You may need to set CMAKE_PREFIX_PATH to your corresponding Qt path.

Out of the source build is highly recomended. Eg:
mkdir build
mkdir install
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../install ..
make
make install

#### Using qmake:
qmake
make
make install

You can use out-of-the-source build again:
mkdir build
cd build
qmake ..
make && make install

#### Cross Compiling with QT5 for Windows on Ubuntu Linux
Based on [this](https://stackoverflow.com/questions/10934683/how-do-i-configure-qt-for-cross-compilation-from-linux-to-windows-target):

1. sudo apt-get install bison cmake flex intltool libtool scons
2. git clone https://github.com/mxe/mxe.git
3. cd mxe && make qt5
4. export PATH=<mxe root>/usr/bin:$PATH
5. Change to build directory parallel to PC CryptoStickGUI directory, e.g. build-CryptoStickGUI-Win32-release
6. <mxe root>/usr/i686-pc-mingw32/qt5/bin/qmake -spec <mxe root>/usr/i686-pc-mingw32/qt5/mkspecs/win32-g++ -o Makefile ../cryptostick.utility/CryptoStickGUI_QT5.pro
7. make
8. optional: use upx to compress the executable


#### Compiling for MAC OS
1. Use Qt to compile the Nitrokey App
2. Navigate to <build_dir>/CryptoStickGUI.app/Contents
3. Create a .dmg file
   Go to the build directory and use
     macdeployqt CryptoStickGUI.app/ -dmg
   CryptoStickGUI.dmg file will be created at the same folder. This is the final file for distributing the App on Mac OS
4. Compress the .dmg package:
   * Open Disk Utility
   * Select the dmg package from left column (or drag'n'drop)
   * Select Convert, check "compressed" option and then "Save"

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


