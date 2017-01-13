//
// Created by sz on 09.01.17.
//

#include <libnitrokey/include/NitrokeyManager.h>
#include "libada.h"
#include <mutex>

std::mutex mex_dev_com;


std::shared_ptr <libada> libada::_instance = nullptr;

using nm = nitrokey::NitrokeyManager;

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
  return 0; //FIXME get real version
}

int libada::getMinorFirmwareVersion() {
  return nm::instance()->get_minor_firmware_version();
}

int libada::getPasswordRetryCount() {
  if (nm::instance()->is_connected())
    return nm::instance()->get_admin_retry_count();
  return 99;
}

int libada::getUserPasswordRetryCount() {
  if (nm::instance()->is_connected()){
    return nm::instance()->get_user_retry_count();
  }
  return 99;
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
  return 42;
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

bool libada::isDeviceConnected() const throw() {
  auto conn = nm::instance()->is_connected();
  return conn;
}

bool libada::isDeviceInitialized() {
  return false;
}

bool libada::isStorageDeviceConnected() const throw() {
  return nm::instance()->get_connected_device_model() == nitrokey::DeviceModel::STORAGE;
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
