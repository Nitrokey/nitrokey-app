//
// Created by sz on 09.01.17.
//

#include <libnitrokey/include/NitrokeyManager.h>
#include "libada.h"

//#include <mutex>
//std::mutex mex_dev_com;


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
    return nm::instance()->get_serial_number();
}

#include <QCache>
#include <QtCore/QMutex>
#include "libnitrokey/include/CommandFailedException.h"
#include "hotpslot.h"

std::string libada::getTOTPSlotName(const int i) {
  static QMutex mut;
  QMutexLocker locker(&mut);
  static QCache<int, std::string> cache;
  if (cache.contains(i)){
    return *cache[i];
  }
  try{
    const auto slot_name = nm::instance()->get_totp_slot_name(i);
    cache.insert(i, new std::string(slot_name));
  }
  catch (CommandFailedException &e){
    if (!e.reason_slot_not_programmed())
      throw;
    cache.insert(i, new std::string("(empty)"));
  }
  return *cache[i];
}

std::string libada::getHOTPSlotName(const int i) {
  static QMutex mut;
  QMutexLocker locker(&mut);
  static QCache<int, std::string> cache;
  if (cache.contains(i)){
    return *cache[i];
  }
  try{
    const auto slot_name = nm::instance()->get_hotp_slot_name(i);
    cache.insert(i, new std::string(slot_name));
  }
  catch (CommandFailedException &e){
    if (!e.reason_slot_not_programmed())
      throw;
    cache.insert(i, new std::string("(empty)"));
  }
  return *cache[i];
}

std::string libada::getPWSSlotName(const int i) {
  return "";
  return nm::instance()->get_password_safe_slot_name(i);
}

bool libada::getPWSSlotStatus(const int i) {
  return false;
  return nm::instance()->get_password_safe_slot_status()[i];
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
  return true;
}

bool libada::isStorageDeviceConnected() const throw() {
  return nm::instance()->is_connected() &&
          nm::instance()->get_connected_device_model() == nitrokey::DeviceModel::STORAGE;
}

bool libada::isPasswordSafeAvailable() {
  return false;
}

bool libada::isPasswordSafeUnlocked() {
  return false;
}

bool libada::isTOTPSlotProgrammed(const int i) {
  return getTOTPSlotName(i).find("empty") == std::string::npos; //FIXME
}

bool libada::isHOTPSlotProgrammed(const int i) {
  return getHOTPSlotName(i).find("empty") == std::string::npos; //FIXME
}

void libada::writeToOTPSlot(const OTPSlot &otpconf, const QString &tempPassword) {
  bool res;
  switch(otpconf.type){
    case OTPSlot::OTPType::HOTP: {
      res = nm::instance()->write_HOTP_slot(otpconf.slotNumber, otpconf.slotName, otpconf.secret, otpconf.interval,
        otpconf.config_st.useEightDigits, otpconf.config_st.useEnter, otpconf.config_st.useTokenID,
      otpconf.tokenID, tempPassword.toLatin1().constData());
    }
      break;
    case OTPSlot::OTPType::TOTP:
      res = nm::instance()->write_TOTP_slot(otpconf.slotNumber, otpconf.slotName, otpconf.secret, otpconf.interval,
                                otpconf.config_st.useEightDigits, otpconf.config_st.useEnter, otpconf.config_st.useTokenID,
                                otpconf.tokenID, tempPassword.toLatin1().constData());
      break;

  }
//  nm::write_HOTP_slot()
}

bool libada::is_nkpro_07_rtm1() {
  return true;
}

bool libada::is_secret320_supported() {
  return false;
}

int libada::getTOTPCode(int i, char *string) {
  return nm::instance()->get_TOTP_code(i, 0, 0, 0, string);
}

int libada::getHOTPCode(int i, char* string) {
  return nm::instance()->get_HOTP_code(i, string);
}

int libada::eraseHOTPSlot(const int i, char *string) {
  return nm::instance()->erase_hotp_slot(i, string);
}

int libada::eraseTOTPSlot(const int i, char *string) {
  return nm::instance()->erase_totp_slot(i, string);
}
