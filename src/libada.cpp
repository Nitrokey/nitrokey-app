//
// Created by sz on 09.01.17.
//

#include "libada.h"

std::shared_ptr <libada> libada::_instance = nullptr;

std::shared_ptr<libada> libada::i() {
  if (_instance == nullptr){
    _instance = std::make_shared<libada>();
  }
  return _instance;
}

libada::libada() {

}

libada::~libada() {

}

int libada::getMajorFirmwareVersion() {
  return 0;
}

int libada::getMinorFirmwareVersion() {
  return 0;
}

int libada::getPasswordRetryCount() {
  return 0;
}

int libada::getUserPasswordRetryCount() {
  return 0;
}

std::string libada::getCardSerial() {
  return std::__cxx11::string();
}

std::string libada::getTOTPSlotName(const int i) {
  return std::__cxx11::string();
}

std::string libada::getHOTPSlotName(const int i) {
  return std::__cxx11::string();
}

std::string libada::getPWSSlotName(const int i) {
  return std::__cxx11::string();
}

bool libada::getPWSSlotStatus(const int i) {
  return false;
}

void libada::erasePWSSlot(const int i) {

}

int libada::getStorageInfoData() {
  return 0;
}

int libada::getStorageSDCardSizeGB() {
  return 0;
}

int libada::setUserPIN() {
  return 0;
}

int libada::setAdminPIN() {
  return 0;
}

int libada::setStorageUpdatePassword() {
  return 0;
}

bool libada::isDeviceConnected() {
  return false;
}

bool libada::isDeviceInitialized() {
  return false;
}

bool libada::isStorageDeviceConnected() {
  return false;
}

bool libada::isPasswordSafeAvailable() {
  return false;
}

bool libada::isPasswordSafeUnlocked() {
  return false;
}

bool libada::isTOTPSlotProgrammed(const int i) {
  return false;
}

bool libada::isHOTPSlotProgrammed(const int i) {
  return false;
}

void libada::writeToOTPSlot(const OTPSlot &otpconf) {

}

bool libada::is_nkpro_07_rtm1() {
  return false;
}

bool libada::is_secret320_supported() {
  return false;
}
