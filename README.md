= Crypto Stick Tool =
Crypto Stick Tool runs on Windows, Linux and Mac OS. It has been created with Qt Creator 2.4.1 with Qt 4.7.4 and MinGW 4.4.

The implementation is compatible to the Google Authenticator application which can be used for testing purposes. See http://google-authenticator.googlecode.com/git/libpam/totp.html

Using the application under Linux also requires root privileges, or configuration of device privileges in udev (because of USB communication).

To compile the Crypto Stick Tool under Linux install the package libusb-1.0.0-dev and QT Creator. You may need to add to the .pro file:
QMAKE_CXXFLAGS= -I/usr/include/libusb-1.0
QMAKE_CFLAGS= -I/usr/include/libusb-1.0

== Compiling on Ubuntu Linux==
Prerequisite: sudo apt-get install qt4-default libqt4-dev libusb-1.0.0-dev qtcreator
Use QT Creator for compilation or perform the following steps:
1) Change to build directory parallel to PC CryptoStickGUI directory
2) For 64 bit system: /usr/bin/qmake-qt4 -spec /usr/share/qt4/mkspecs/linux-g++-64 -o Makefile ../CryptoStickGUI/CryptoStickGUI.pro
For 32 bit system: /usr/bin/qmake-qt4 -spec /usr/share/qt4/mkspecs/linux-g++-32 -o Makefile ../CryptoStickGUI/CryptoStickGUI.pro
3) make

== Cross Compiling for Windows under Ubuntu Linux ==
Based on: https://stackoverflow.com/questions/10934683/how-do-i-configure-qt-for-cross-compilation-from-linux-to-windows-target
1) sudo make install bison cmake flex intltool libtool scons
2) git clone https://github.com/mxe/mxe.git
3) cd mxe && make qt
4) export PATH=<mxe root>/usr/bin:$PATH
5) Change to build directory parallel to PC CryptoStickGUI directory, e.g. build-CryptoStickGUI-Win32-release
6) <mxe root>/usr/i686-pc-mingw32/qt/bin/qmake -spec <mxe root>/usr/i686-pc-mingw32/qt/mkspecs/default -o Makefile ../CryptoStickGUI/CryptoStickGUI.pro
7) make

=Firmware=
In the makefile you have to change the tool names according to your toolchain
I changed them to "CC = arm-none-eabi-gcc" etc.
In hw_config.c and platform_config.h you have to set up the USB
disconnect pin, because I had to change it for my devboard.

=Internals=
All configuration data including OTP secrets are stored in clear text in the flash of Crypto Stick's Microcontroller. This is not tamper resistant and may only be used for low to medium security requirements.

By default the OTP serial number is OpenPGP Card's serial number. It can be changed within the GUI. The USB device serial number is set to the card's serial number when the device powers up.

Keyboard Layout: The user will input the token ID values as he wants them displayed, then the gui will translate them to keycodes of the selected layout. The keycodes will be stored on the device, along with a number saying which layout was used, this number is important when the GUI application reads the conifg back from the device (to translate the keycodes back into characters).

The report protocol is described a little here:
https://www.assembla.com/code/cryptostick/git/nodes/master/firmware/src/INC/report_protocol.h
The HID reports are set to 64 bytes. The "output report" is what you get from the device. When you send a report (command), the first byte sets the command type, then you have 59 bytes for your data, and the last 4 bytes are the CRC32 of the whole report.

On the GUI side, you can find examples of sending commands and receiving responses here:
https://www.assembla.com/code/cryptostick/git/nodes/master/PC%20software/CryptoStickGUI/device.cpp

First you create an object of class Command, for that you need a number for the command type, a buffer of data you want to send and the length of that buffer. Then you just use Device::sendCommand(Command *cmd), the CRC32 will be calculated automatically.
To get a response, you just create a new object of class Response and do Response::getResponse(Device *device) and then you can analyze contents of the response object, already divided into fields:
https://www.assembla.com/code/cryptostick/git/nodes/master/PC%20software/CryptoStickGUI/response.h

=To Dos=
Other than libusb, HID API has the advantage that it uses native drivers on OSX. Consider migrating to it. https://github.com/signal11/hidapi 

* Remove the windows showing the actual OTP values and copy the OTP value to the clipboard directly.

* Change name to "Crypto Stick Tool". 

* If the user enteres a wrong PIN, the dialog should state how many retries are left until the card gets locked.

* incorporate help "?" buttons, as indicated in my mockup

* Make USB description and token ID configurable, e.g. through config file?


