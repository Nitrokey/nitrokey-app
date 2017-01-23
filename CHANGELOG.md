# Change Log

## [v0.6.3](https://github.com/Nitrokey/nitrokey-app/tree/v0.6.3) (2017-01-23)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.6.2...v0.6.3)

**Closed issues:**

- Do not enable new HID protocol for Storage yet [\#218](https://github.com/Nitrokey/nitrokey-app/issues/218)
- nitrokey-app keeps asking for admin PIN when trying to configure TOTP or HOTP slot [\#211](https://github.com/Nitrokey/nitrokey-app/issues/211)

**Merged pull requests:**

- Disable support of new HID protocol for Storage [\#219](https://github.com/Nitrokey/nitrokey-app/pull/219) ([szszszsz](https://github.com/szszszsz))
- Debian packaging rules changes [\#217](https://github.com/Nitrokey/nitrokey-app/pull/217) ([szszszsz](https://github.com/szszszsz))

## [v0.6.2](https://github.com/Nitrokey/nitrokey-app/tree/v0.6.2) (2017-01-03)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.6.1...v0.6.2)

**Fixed bugs:**

- OTP not working on Windows [\#214](https://github.com/Nitrokey/nitrokey-app/issues/214)

**Merged pull requests:**

- Bump version to 0.6.2 [\#216](https://github.com/Nitrokey/nitrokey-app/pull/216) ([szszszsz](https://github.com/szszszsz))
- Pack union for writing OTP slot data [\#215](https://github.com/Nitrokey/nitrokey-app/pull/215) ([szszszsz](https://github.com/szszszsz))
- Do not block on waiting for symlink to storage being available [\#213](https://github.com/Nitrokey/nitrokey-app/pull/213) ([szszszsz](https://github.com/szszszsz))
- Break asking PIN loop when user cancels it [\#212](https://github.com/Nitrokey/nitrokey-app/pull/212) ([szszszsz](https://github.com/szszszsz))
- Fix compilation warnings [\#208](https://github.com/Nitrokey/nitrokey-app/pull/208) ([szszszsz](https://github.com/szszszsz))

## [v0.6.1](https://github.com/Nitrokey/nitrokey-app/tree/v0.6.1) (2016-12-01)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.6...v0.6.1)

**Merged pull requests:**

- Use native settings handler [\#206](https://github.com/Nitrokey/nitrokey-app/pull/206) ([szszszsz](https://github.com/szszszsz))
- Bump version to 0.6.1 [\#205](https://github.com/Nitrokey/nitrokey-app/pull/205) ([szszszsz](https://github.com/szszszsz))
- Add Arabic translation to resource file [\#204](https://github.com/Nitrokey/nitrokey-app/pull/204) ([szszszsz](https://github.com/szszszsz))

## [v0.6](https://github.com/Nitrokey/nitrokey-app/tree/v0.6) (2016-11-28)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.5.1...v0.6)

**Implemented enhancements:**

- Support for Pro stick version 0.8 [\#190](https://github.com/Nitrokey/nitrokey-app/issues/190)

**Closed issues:**

- Add command line parameter to force language [\#199](https://github.com/Nitrokey/nitrokey-app/issues/199)
- OTP secret field's max length changes only with text inside [\#195](https://github.com/Nitrokey/nitrokey-app/issues/195)
- Fedora 23 RPM has conflicts [\#193](https://github.com/Nitrokey/nitrokey-app/issues/193)
- RPM package installs in wrong directory [\#178](https://github.com/Nitrokey/nitrokey-app/issues/178)
- Invisible tray icon on openSUSE Tumbleweed unless run as root  [\#176](https://github.com/Nitrokey/nitrokey-app/issues/176)
- Mac OS X version and missing QT libraries [\#163](https://github.com/Nitrokey/nitrokey-app/issues/163)

**Merged pull requests:**

- Changelog update [\#203](https://github.com/Nitrokey/nitrokey-app/pull/203) ([szszszsz](https://github.com/szszszsz))
- Update README.md [\#202](https://github.com/Nitrokey/nitrokey-app/pull/202) ([szszszsz](https://github.com/szszszsz))
- Add language CLI param [\#200](https://github.com/Nitrokey/nitrokey-app/pull/200) ([szszszsz](https://github.com/szszszsz))
- Bump version to 0.6 [\#198](https://github.com/Nitrokey/nitrokey-app/pull/198) ([szszszsz](https://github.com/szszszsz))
- Support for Nitrokey Pro 0.8 [\#196](https://github.com/Nitrokey/nitrokey-app/pull/196) ([szszszsz](https://github.com/szszszsz))
- Resolve conflict with udev rules \(for Fedora 23\) [\#194](https://github.com/Nitrokey/nitrokey-app/pull/194) ([vladimir-lu](https://github.com/vladimir-lu))
- build: set CMAKE\_BUILD\_TYPE only if not set [\#188](https://github.com/Nitrokey/nitrokey-app/pull/188) ([ignatenkobrain](https://github.com/ignatenkobrain))
- Translations: initial support for Arabic \(ar\) language [\#183](https://github.com/Nitrokey/nitrokey-app/pull/183) ([szszszsz](https://github.com/szszszsz))
- Change RPM package installation dir [\#180](https://github.com/Nitrokey/nitrokey-app/pull/180) ([szszszsz](https://github.com/szszszsz))
- Update README.md [\#179](https://github.com/Nitrokey/nitrokey-app/pull/179) ([szszszsz](https://github.com/szszszsz))
- Update changelog [\#174](https://github.com/Nitrokey/nitrokey-app/pull/174) ([szszszsz](https://github.com/szszszsz))

## [v0.5.1](https://github.com/Nitrokey/nitrokey-app/tree/v0.5.1) (2016-10-08)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.5...v0.5.1)

**Closed issues:**

- Retry counters are not updated before opening About form \(NK Storage\) [\#170](https://github.com/Nitrokey/nitrokey-app/issues/170)
- No failure feedback when user erases OTP slot [\#168](https://github.com/Nitrokey/nitrokey-app/issues/168)
- Confirm erasing PWS slot [\#167](https://github.com/Nitrokey/nitrokey-app/issues/167)
- Display NK Storage capacity in About window [\#162](https://github.com/Nitrokey/nitrokey-app/issues/162)
- Abort action when device is not initialized [\#156](https://github.com/Nitrokey/nitrokey-app/issues/156)
- Buffer overflow while adding OTP [\#155](https://github.com/Nitrokey/nitrokey-app/issues/155)
- Provide binaries for Ubuntu via Launchpad [\#115](https://github.com/Nitrokey/nitrokey-app/issues/115)
- show success dialogue when saving a new static password: [\#57](https://github.com/Nitrokey/nitrokey-app/issues/57)

**Merged pull requests:**

- Fix typo in debian/changelog [\#173](https://github.com/Nitrokey/nitrokey-app/pull/173) ([szszszsz](https://github.com/szszszsz))
- Bump version to 0.5.1 [\#172](https://github.com/Nitrokey/nitrokey-app/pull/172) ([szszszsz](https://github.com/szszszsz))
- Confirm erasing slot in Password Safe. Handle confirmation properly in OTP. [\#171](https://github.com/Nitrokey/nitrokey-app/pull/171) ([szszszsz](https://github.com/szszszsz))
- Abort sending command to uninitialized device [\#169](https://github.com/Nitrokey/nitrokey-app/pull/169) ([szszszsz](https://github.com/szszszsz))
- Show feedback message after saving password to Password Safe [\#166](https://github.com/Nitrokey/nitrokey-app/pull/166) ([szszszsz](https://github.com/szszszsz))
- Add to About window stick's storage capacity [\#165](https://github.com/Nitrokey/nitrokey-app/pull/165) ([szszszsz](https://github.com/szszszsz))
- Avoid buffer overflow during hex\<-\>base32 conversion [\#164](https://github.com/Nitrokey/nitrokey-app/pull/164) ([szszszsz](https://github.com/szszszsz))
- Remove issues closed as duplicate from the changelog [\#160](https://github.com/Nitrokey/nitrokey-app/pull/160) ([szszszsz](https://github.com/szszszsz))
- Add CHANGELOG.md generated with github-changelog-generator [\#159](https://github.com/Nitrokey/nitrokey-app/pull/159) ([szszszsz](https://github.com/szszszsz))

## [v0.5](https://github.com/Nitrokey/nitrokey-app/tree/v0.5) (2016-09-28)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.4...v0.5)

**Implemented enhancements:**

- Improve dialogue to setup hidden volumes [\#106](https://github.com/Nitrokey/nitrokey-app/issues/106)

**Fixed bugs:**

- NK Pro: Impossible to write consecutive HOTP codes [\#62](https://github.com/Nitrokey/nitrokey-app/issues/62)
- Exception after checking HOTP password with open configuration form [\#30](https://github.com/Nitrokey/nitrokey-app/issues/30)

**Closed issues:**

- Update translations before release 0.5 [\#150](https://github.com/Nitrokey/nitrokey-app/issues/150)
- Sequential erasing of OTP slots not working  [\#146](https://github.com/Nitrokey/nitrokey-app/issues/146)
- Remove test OTP controls from configuration window [\#145](https://github.com/Nitrokey/nitrokey-app/issues/145)
- PIN protect OTP not working on first request with Storage [\#144](https://github.com/Nitrokey/nitrokey-app/issues/144)
- Change  PIN does not work [\#143](https://github.com/Nitrokey/nitrokey-app/issues/143)
- Crash when showing message box [\#142](https://github.com/Nitrokey/nitrokey-app/issues/142)
- Warn about OTP secrets starting from 0x00 [\#139](https://github.com/Nitrokey/nitrokey-app/issues/139)
- Send 20 null bytes as a secret on OTP edit [\#138](https://github.com/Nitrokey/nitrokey-app/issues/138)
- Password Safe: Store Unicode characters in passwords [\#131](https://github.com/Nitrokey/nitrokey-app/issues/131)
- Notification Window after initialization [\#130](https://github.com/Nitrokey/nitrokey-app/issues/130)
- Make `Forget PIN after 10 minutes` option work again [\#109](https://github.com/Nitrokey/nitrokey-app/issues/109)
- Add warning that Firmware Password can not be reset [\#103](https://github.com/Nitrokey/nitrokey-app/issues/103)
- Add guidelines for contributing [\#75](https://github.com/Nitrokey/nitrokey-app/issues/75)
- Improve usability of hidden volume setup form [\#72](https://github.com/Nitrokey/nitrokey-app/issues/72)
- Check feedback for password changing with Nitrokey Pro [\#68](https://github.com/Nitrokey/nitrokey-app/issues/68)
- Check clipboard clearing mechanism for password safe [\#54](https://github.com/Nitrokey/nitrokey-app/issues/54)
- Various packaging related problems [\#11](https://github.com/Nitrokey/nitrokey-app/issues/11)

**Merged pull requests:**

- Bump version to 0.5.0 [\#158](https://github.com/Nitrokey/nitrokey-app/pull/158) ([szszszsz](https://github.com/szszszsz))
- Update translations before 0.5 release [\#157](https://github.com/Nitrokey/nitrokey-app/pull/157) ([szszszsz](https://github.com/szszszsz))
- Remove locations from translation files [\#154](https://github.com/Nitrokey/nitrokey-app/pull/154) ([szszszsz](https://github.com/szszszsz))
- Ask for device's reinsertion when its locked [\#153](https://github.com/Nitrokey/nitrokey-app/pull/153) ([szszszsz](https://github.com/szszszsz))
- Warn user about secret starting from 0x00 [\#152](https://github.com/Nitrokey/nitrokey-app/pull/152) ([szszszsz](https://github.com/szszszsz))
- Refactoring: mainWindow::on\_writeButton [\#151](https://github.com/Nitrokey/nitrokey-app/pull/151) ([szszszsz](https://github.com/szszszsz))
- Code refactoring in an attempt to fix \#142 [\#149](https://github.com/Nitrokey/nitrokey-app/pull/149) ([szszszsz](https://github.com/szszszsz))
- Remove test HOTP TOTP from configuration window \#145 [\#148](https://github.com/Nitrokey/nitrokey-app/pull/148) ([szszszsz](https://github.com/szszszsz))
- remember PIN for NK Pro while requesting PIN protected OTP [\#147](https://github.com/Nitrokey/nitrokey-app/pull/147) ([szszszsz](https://github.com/szszszsz))
- Readme - update links in internals section [\#141](https://github.com/Nitrokey/nitrokey-app/pull/141) ([szszszsz](https://github.com/szszszsz))
- Issue 130 Reinsert notification after initialization [\#134](https://github.com/Nitrokey/nitrokey-app/pull/134) ([szszszsz](https://github.com/szszszsz))
- Issue 131 Unicode characters for Password Safe [\#133](https://github.com/Nitrokey/nitrokey-app/pull/133) ([szszszsz](https://github.com/szszszsz))
- Issue 11 packaging related problems [\#132](https://github.com/Nitrokey/nitrokey-app/pull/132) ([szszszsz](https://github.com/szszszsz))
- Build package on launchpad 2 [\#129](https://github.com/Nitrokey/nitrokey-app/pull/129) ([szszszsz](https://github.com/szszszsz))
- Travis: remove qt5declarative depend package from build [\#128](https://github.com/Nitrokey/nitrokey-app/pull/128) ([szszszsz](https://github.com/szszszsz))
- Adjust debian package creating script for launchpad.net [\#127](https://github.com/Nitrokey/nitrokey-app/pull/127) ([szszszsz](https://github.com/szszszsz))
- Hidden setup usability fix [\#126](https://github.com/Nitrokey/nitrokey-app/pull/126) ([szszszsz](https://github.com/szszszsz))
- Add contribution guidelines [\#125](https://github.com/Nitrokey/nitrokey-app/pull/125) ([szszszsz](https://github.com/szszszsz))
- Add license copy to repository - GPLv3 [\#124](https://github.com/Nitrokey/nitrokey-app/pull/124) ([szszszsz](https://github.com/szszszsz))
- Add missing udev entry for Nitrokey Storage [\#123](https://github.com/Nitrokey/nitrokey-app/pull/123) ([szszszsz](https://github.com/szszszsz))
- Update udev rules [\#122](https://github.com/Nitrokey/nitrokey-app/pull/122) ([szszszsz](https://github.com/szszszsz))
- Code reformat only for modified files vs master branch [\#120](https://github.com/Nitrokey/nitrokey-app/pull/120) ([szszszsz](https://github.com/szszszsz))
- Issue 54 clipboard clearing [\#117](https://github.com/Nitrokey/nitrokey-app/pull/117) ([szszszsz](https://github.com/szszszsz))
- Add warning that Firmware Password can not be reset \#103 [\#116](https://github.com/Nitrokey/nitrokey-app/pull/116) ([szszszsz](https://github.com/szszszsz))
- Debian packaging patch [\#112](https://github.com/Nitrokey/nitrokey-app/pull/112) ([szszszsz](https://github.com/szszszsz))
- Issue 68 password feedback pro [\#107](https://github.com/Nitrokey/nitrokey-app/pull/107) ([szszszsz](https://github.com/szszszsz))
- Add missing forward declaration [\#105](https://github.com/Nitrokey/nitrokey-app/pull/105) ([ansiwen](https://github.com/ansiwen))
- prevent make install dirty the root filesystem, stick to the prefix [\#89](https://github.com/Nitrokey/nitrokey-app/pull/89) ([szszszsz](https://github.com/szszszsz))
- Update travis configuration for application build [\#69](https://github.com/Nitrokey/nitrokey-app/pull/69) ([szszszsz](https://github.com/szszszsz))

## [v0.4](https://github.com/Nitrokey/nitrokey-app/tree/v0.4) (2016-07-02)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.3.2...v0.4)

**Implemented enhancements:**

- document CLI interface [\#24](https://github.com/Nitrokey/nitrokey-app/issues/24)

**Fixed bugs:**

- Only three passwords displayed [\#78](https://github.com/Nitrokey/nitrokey-app/issues/78)
- NK Storage: Changing Firmware Password doesn't work [\#71](https://github.com/Nitrokey/nitrokey-app/issues/71)
- NK Storage: Counter value is not written for HOTP [\#60](https://github.com/Nitrokey/nitrokey-app/issues/60)
- The tray icon isn't displayed [\#43](https://github.com/Nitrokey/nitrokey-app/issues/43)
- Application not compiling under qmake [\#40](https://github.com/Nitrokey/nitrokey-app/issues/40)
- No feedback after changing password [\#38](https://github.com/Nitrokey/nitrokey-app/issues/38)

**Closed issues:**

- Limit password length for firmware password in password dialog [\#100](https://github.com/Nitrokey/nitrokey-app/issues/100)
- Check feedback for firmware password changing with Nitrokey Storage [\#97](https://github.com/Nitrokey/nitrokey-app/issues/97)
- Can't unlock password safe [\#96](https://github.com/Nitrokey/nitrokey-app/issues/96)
- Can't configure hidden volume [\#95](https://github.com/Nitrokey/nitrokey-app/issues/95)
- Readme: remove section about building from sources with QT4 [\#85](https://github.com/Nitrokey/nitrokey-app/issues/85)
- Only first TOTP is valid [\#82](https://github.com/Nitrokey/nitrokey-app/issues/82)
- USB 3.0: Updating progress bar during clearing SD card is not working [\#76](https://github.com/Nitrokey/nitrokey-app/issues/76)
- NK Storage: About dialog does not show current retry counts [\#66](https://github.com/Nitrokey/nitrokey-app/issues/66)
- Show available password retry count in change password form [\#65](https://github.com/Nitrokey/nitrokey-app/issues/65)
- Warning displayed when unlocking an encrypted NTFS partition [\#58](https://github.com/Nitrokey/nitrokey-app/issues/58)
- NK Storage: Erasing HOTP slot results in LED staying red on Nitrokey [\#56](https://github.com/Nitrokey/nitrokey-app/issues/56)
- 'Passwords' submenu should not be shown when no passwords are available [\#53](https://github.com/Nitrokey/nitrokey-app/issues/53)
- NK Storage: stick is reconnecting while wiping out SD card [\#48](https://github.com/Nitrokey/nitrokey-app/issues/48)
- NK Storage: Display menu entry "lock device" only when device is unlocked [\#46](https://github.com/Nitrokey/nitrokey-app/issues/46)
- NK Storage: Combine menu entries Generate Keys and Initialize Storage WIth Random Data [\#45](https://github.com/Nitrokey/nitrokey-app/issues/45)
- Implement better detection of unpartitioned encrypted volume [\#42](https://github.com/Nitrokey/nitrokey-app/issues/42)
- Check new translations  [\#41](https://github.com/Nitrokey/nitrokey-app/issues/41)
- Decouple counter and seed edit controls in TOTP/HOTP configuration form [\#31](https://github.com/Nitrokey/nitrokey-app/issues/31)
- Refocus App on OS X [\#23](https://github.com/Nitrokey/nitrokey-app/issues/23)
- RPM Package conflicts with system packages [\#18](https://github.com/Nitrokey/nitrokey-app/issues/18)
- Specify min/max pin length in UI [\#15](https://github.com/Nitrokey/nitrokey-app/issues/15)
- Admin PIN with more than 20 chars [\#12](https://github.com/Nitrokey/nitrokey-app/issues/12)
- Storage: verify Update PIN when "enable firmware update" is selected [\#2](https://github.com/Nitrokey/nitrokey-app/issues/2)

**Merged pull requests:**

- Translations pack [\#104](https://github.com/Nitrokey/nitrokey-app/pull/104) ([szszszsz](https://github.com/szszszsz))
- Allow firmware password to have at most CS20\_MAX\_UPDATE\_PASSWORD\_LEN [\#102](https://github.com/Nitrokey/nitrokey-app/pull/102) ([szszszsz](https://github.com/szszszsz))
- Correct firmware password limit for update command [\#101](https://github.com/Nitrokey/nitrokey-app/pull/101) ([szszszsz](https://github.com/szszszsz))
- Issue 97 firmware password feedback no debug [\#99](https://github.com/Nitrokey/nitrokey-app/pull/99) ([szszszsz](https://github.com/szszszsz))
- Take status from device after AES keys initialization [\#98](https://github.com/Nitrokey/nitrokey-app/pull/98) ([szszszsz](https://github.com/szszszsz))
- Issue 78 only three passwords displayed [\#93](https://github.com/Nitrokey/nitrokey-app/pull/93) ([szszszsz](https://github.com/szszszsz))
- Issue 45 NK Storage: Combine menu entries Generate Keys and Initialize Storage WIth Random Data  [\#91](https://github.com/Nitrokey/nitrokey-app/pull/91) ([szszszsz](https://github.com/szszszsz))
- Issue 82 Only first TOTP is valid when OTP is protected by user PIN [\#88](https://github.com/Nitrokey/nitrokey-app/pull/88) ([szszszsz](https://github.com/szszszsz))
- Revert "Issue 48 disable reconnecting messages while wiping" [\#83](https://github.com/Nitrokey/nitrokey-app/pull/83) ([szszszsz](https://github.com/szszszsz))
- Handle Factory Reset CLI command [\#81](https://github.com/Nitrokey/nitrokey-app/pull/81) ([szszszsz](https://github.com/szszszsz))
- Extend CLI with stick update mode [\#79](https://github.com/Nitrokey/nitrokey-app/pull/79) ([szszszsz](https://github.com/szszszsz))
- Issue 48 disable reconnecting messages while wiping [\#70](https://github.com/Nitrokey/nitrokey-app/pull/70) ([szszszsz](https://github.com/szszszsz))
- Issue 38 no feedback after changing password [\#67](https://github.com/Nitrokey/nitrokey-app/pull/67) ([szszszsz](https://github.com/szszszsz))
- Issue 60 handle both firmwares for hotp [\#64](https://github.com/Nitrokey/nitrokey-app/pull/64) ([szszszsz](https://github.com/szszszsz))
- Correcting solution for issue \#45 [\#63](https://github.com/Nitrokey/nitrokey-app/pull/63) ([szszszsz](https://github.com/szszszsz))
- Issue 31 decouple counter [\#61](https://github.com/Nitrokey/nitrokey-app/pull/61) ([szszszsz](https://github.com/szszszsz))
- Coding style for only modified files [\#59](https://github.com/Nitrokey/nitrokey-app/pull/59) ([szszszsz](https://github.com/szszszsz))
- Do not show empty passwords submenu [\#55](https://github.com/Nitrokey/nitrokey-app/pull/55) ([szszszsz](https://github.com/szszszsz))
- Reformatting code with clang-format [\#52](https://github.com/Nitrokey/nitrokey-app/pull/52) ([szszszsz](https://github.com/szszszsz))
- Revert "Add console in CONFIG   += qt console" [\#51](https://github.com/Nitrokey/nitrokey-app/pull/51) ([szszszsz](https://github.com/szszszsz))
- Hiding "Lock device" from menu when no unlocked disks are available [\#50](https://github.com/Nitrokey/nitrokey-app/pull/50) ([szszszsz](https://github.com/szszszsz))
- Combine generating AES keys and sd card initialization  [\#49](https://github.com/Nitrokey/nitrokey-app/pull/49) ([szszszsz](https://github.com/szszszsz))
- Workaround for issue 43 [\#47](https://github.com/Nitrokey/nitrokey-app/pull/47) ([szszszsz](https://github.com/szszszsz))

## [v0.3.2](https://github.com/Nitrokey/nitrokey-app/tree/v0.3.2) (2016-04-11)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.2...v0.3.2)

**Fixed bugs:**

- Not working HOTP with Nitrokey Pro - "BUG: nitrokey HOTP BROKEN" [\#27](https://github.com/Nitrokey/nitrokey-app/issues/27)

**Closed issues:**

- Hidden volume is volatile [\#28](https://github.com/Nitrokey/nitrokey-app/issues/28)
- Please push tag v0.2 [\#8](https://github.com/Nitrokey/nitrokey-app/issues/8)
- provide writeToHOTPSlot as shared library [\#5](https://github.com/Nitrokey/nitrokey-app/issues/5)
- Conditionally compile libappindicator support. [\#3](https://github.com/Nitrokey/nitrokey-app/issues/3)

**Merged pull requests:**

- Fix for issue 28 [\#34](https://github.com/Nitrokey/nitrokey-app/pull/34) ([szszszsz](https://github.com/szszszsz))
- Fixing HOTP bug \#27 [\#32](https://github.com/Nitrokey/nitrokey-app/pull/32) ([szszszsz](https://github.com/szszszsz))
- fix typo in icon filename [\#13](https://github.com/Nitrokey/nitrokey-app/pull/13) ([samjuvonen](https://github.com/samjuvonen))

## [v0.2](https://github.com/Nitrokey/nitrokey-app/tree/v0.2) (2015-09-08)
[Full Changelog](https://github.com/Nitrokey/nitrokey-app/compare/v0.1...v0.2)

**Merged pull requests:**

- Fixed typo in target.path, added note to README regarding non-KDE deskto... [\#1](https://github.com/Nitrokey/nitrokey-app/pull/1) ([dfj](https://github.com/dfj))

## [v0.1](https://github.com/Nitrokey/nitrokey-app/tree/v0.1) (2014-12-02)


\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*
