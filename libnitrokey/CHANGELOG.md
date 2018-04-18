# Change Log

## [v3.2](https://github.com/Nitrokey/libnitrokey/tree/v3.2) (2018-01-16)
[Full Changelog](https://github.com/Nitrokey/libnitrokey/compare/v3.1...v3.2)

**Closed issues:**

- Communication exceptions are not catched in C API [\#89](https://github.com/Nitrokey/libnitrokey/issues/89)
- Devices are not detected under Windows [\#88](https://github.com/Nitrokey/libnitrokey/issues/88)
- Python bindings example is not working [\#82](https://github.com/Nitrokey/libnitrokey/issues/82)
- Missing NK\_C\_API.h in install includes [\#77](https://github.com/Nitrokey/libnitrokey/issues/77)
- Handle time-taking commands [\#67](https://github.com/Nitrokey/libnitrokey/issues/67)

## [v3.1](https://github.com/Nitrokey/libnitrokey/tree/v3.1) (2017-10-11)
[Full Changelog](https://github.com/Nitrokey/libnitrokey/compare/v3.0...v3.1)

## [v3.0](https://github.com/Nitrokey/libnitrokey/tree/v3.0) (2017-10-07)
[Full Changelog](https://github.com/Nitrokey/libnitrokey/compare/v2.0...v3.0)

**Implemented enhancements:**

- Support for NK Pro 0.8 [\#51](https://github.com/Nitrokey/libnitrokey/issues/51)

**Closed issues:**

- tests are failing [\#71](https://github.com/Nitrokey/libnitrokey/issues/71)
- cmake doesn't install any headers [\#70](https://github.com/Nitrokey/libnitrokey/issues/70)
- SONAME versioning [\#69](https://github.com/Nitrokey/libnitrokey/issues/69)
- Read slot returns invalid counter value for Storage [\#59](https://github.com/Nitrokey/libnitrokey/issues/59)
- Return OTP codes as strings [\#57](https://github.com/Nitrokey/libnitrokey/issues/57)
- Fix compilation warnings [\#56](https://github.com/Nitrokey/libnitrokey/issues/56)
- Add Travis support [\#48](https://github.com/Nitrokey/libnitrokey/issues/48)
- Correct get\_time [\#27](https://github.com/Nitrokey/libnitrokey/issues/27)
- Move from Makefile to CMake [\#18](https://github.com/Nitrokey/libnitrokey/issues/18)

## [v2.0](https://github.com/Nitrokey/libnitrokey/tree/v2.0) (2016-12-12)
[Full Changelog](https://github.com/Nitrokey/libnitrokey/compare/v1.0...v2.0)

**Implemented enhancements:**

- Support for Nitrokey Storage - Nitrokey Storage commands [\#14](https://github.com/Nitrokey/libnitrokey/issues/14)
- Support for Nitrokey Storage - Nitrokey Pro commands [\#13](https://github.com/Nitrokey/libnitrokey/issues/13)

**Fixed bugs:**

- Fails to compile on ubuntu 16.04 [\#46](https://github.com/Nitrokey/libnitrokey/issues/46)
- C++ tests do not compile [\#45](https://github.com/Nitrokey/libnitrokey/issues/45)
- HOTP counter values limited to 8bit [\#44](https://github.com/Nitrokey/libnitrokey/issues/44)
- Device is not released after library function disconnect call [\#43](https://github.com/Nitrokey/libnitrokey/issues/43)

**Closed issues:**

- Compilation error on G++6 \(flexible array member\) [\#49](https://github.com/Nitrokey/libnitrokey/issues/49)
- No library function for getting device's serial number [\#33](https://github.com/Nitrokey/libnitrokey/issues/33)
- Pass binary data \(OTP secrets\) coded as hex values [\#31](https://github.com/Nitrokey/libnitrokey/issues/31)
- set\_time not always work [\#29](https://github.com/Nitrokey/libnitrokey/issues/29)

**Merged pull requests:**

- Support for Nitrokey Pro 0.8 [\#53](https://github.com/Nitrokey/libnitrokey/pull/53) ([szszszsz](https://github.com/szszszsz))
- Support Nitrokey Storage [\#52](https://github.com/Nitrokey/libnitrokey/pull/52) ([szszszsz](https://github.com/szszszsz))
- Fix compilation G++6 error [\#50](https://github.com/Nitrokey/libnitrokey/pull/50) ([szszszsz](https://github.com/szszszsz))
- Fix compilation warning and error under G++ [\#47](https://github.com/Nitrokey/libnitrokey/pull/47) ([szszszsz](https://github.com/szszszsz))
- Support Pro stick commands on Storage device [\#42](https://github.com/Nitrokey/libnitrokey/pull/42) ([szszszsz](https://github.com/szszszsz))
- Readme update - dependencies, compilers, format [\#40](https://github.com/Nitrokey/libnitrokey/pull/40) ([szszszsz](https://github.com/szszszsz))
- Issue 31 secret as hex [\#36](https://github.com/Nitrokey/libnitrokey/pull/36) ([szszszsz](https://github.com/szszszsz))
- Function for getting device's serial number in hex. Fixes \#33 [\#34](https://github.com/Nitrokey/libnitrokey/pull/34) ([szszszsz](https://github.com/szszszsz))

## [v1.0](https://github.com/Nitrokey/libnitrokey/tree/v1.0) (2016-08-09)
[Full Changelog](https://github.com/Nitrokey/libnitrokey/compare/v0.9...v1.0)

**Closed issues:**

- Automatically detect any connected stick [\#22](https://github.com/Nitrokey/libnitrokey/issues/22)
- Security check [\#20](https://github.com/Nitrokey/libnitrokey/issues/20)
- Show friendly message when no device is connected and do not abort [\#17](https://github.com/Nitrokey/libnitrokey/issues/17)
- Fix PIN protected OTP for Pro [\#15](https://github.com/Nitrokey/libnitrokey/issues/15)

## [v0.9](https://github.com/Nitrokey/libnitrokey/tree/v0.9) (2016-08-05)
[Full Changelog](https://github.com/Nitrokey/libnitrokey/compare/v0.8...v0.9)

**Closed issues:**

- Add README [\#6](https://github.com/Nitrokey/libnitrokey/issues/6)
- Support configs for OTP slots [\#19](https://github.com/Nitrokey/libnitrokey/issues/19)
- Cover remaining Nitrokey Pro commands in lib and tests [\#16](https://github.com/Nitrokey/libnitrokey/issues/16)

**Merged pull requests:**

- waffle.io Badge [\#21](https://github.com/Nitrokey/libnitrokey/pull/21) ([waffle-iron](https://github.com/waffle-iron))

## [v0.8](https://github.com/Nitrokey/libnitrokey/tree/v0.8) (2016-08-02)
**Implemented enhancements:**

- Change license to LGPLv3 [\#1](https://github.com/Nitrokey/libnitrokey/issues/1)

**Closed issues:**

- Python wrapper for OTP and PINs [\#12](https://github.com/Nitrokey/libnitrokey/issues/12)
- Fails to built with default setting on ubuntu 14.04 [\#11](https://github.com/Nitrokey/libnitrokey/issues/11)
- Check why packet buffers are not zeroed [\#8](https://github.com/Nitrokey/libnitrokey/issues/8)

**Merged pull requests:**

- Reset the HOTP counter [\#9](https://github.com/Nitrokey/libnitrokey/pull/9) ([cornelinux](https://github.com/cornelinux))



\* *This Change Log was automatically generated by [github_changelog_generator](https://github.com/skywinder/Github-Changelog-Generator)*