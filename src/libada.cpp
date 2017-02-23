#include <libnitrokey/include/NitrokeyManager.h>
#include "libada.h"


std::shared_ptr <libada> libada::_instance = nullptr;

using nm = nitrokey::NitrokeyManager;

std::shared_ptr<libada> libada::i() {
  if (_instance == nullptr){
    _instance = std::make_shared<libada>();
  }
  return _instance;
}

libada::libada() {}

libada::~libada() {}

int libada::getMajorFirmwareVersion() {
  return 0; //FIXME get real version
}

int libada::getMinorFirmwareVersion() {
  return nm::instance()->get_minor_firmware_version();
}

int libada::getAdminPasswordRetryCount() {
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

#include <QtCore/QMutex>
#include "libnitrokey/include/CommandFailedException.h"
#include "hotpslot.h"

void libada::on_FactoryReset(){
  cache_PWS_name.clear();
  status_PWS.clear();
  cache_HOTP_name.clear();
  cache_TOTP_name.clear();
}


void libada::on_PWS_save(int slot_no){
  cache_PWS_name.remove(slot_no);
  status_PWS.clear();
}

void libada::on_OTP_save(int slot_no, bool isHOTP){
  if(isHOTP){
    cache_HOTP_name.remove(slot_no);
  } else {
    cache_TOTP_name.remove(slot_no);
  }
}

#include <QDebug>
std::string libada::getTOTPSlotName(const int i) {
  static QMutex mut;
  QMutexLocker locker(&mut);
  if (cache_TOTP_name.contains(i)){
    return *cache_TOTP_name[i];
  }
  try{
    const auto slot_name = nm::instance()->get_totp_slot_name(i);
    cache_TOTP_name.insert(i, new std::string(slot_name));
    free((void *) slot_name);
  }
  catch (CommandFailedException &e){
    if (!e.reason_slot_not_programmed())
      throw;
    cache_TOTP_name.insert(i, new std::string(""));
  }
  catch (DeviceCommunicationException &e){
    //FIXME log!
//    emit DeviceDisconnected();
    qDebug() << __PRETTY_FUNCTION__ << "DeviceCommunicationException";
  }
  return *cache_TOTP_name[i];
}

std::string libada::getHOTPSlotName(const int i) {
  static QMutex mut;
  QMutexLocker locker(&mut);
  if (cache_HOTP_name.contains(i)){
    return *cache_HOTP_name[i];
  }
  try{
    const auto slot_name = nm::instance()->get_hotp_slot_name(i);
    cache_HOTP_name.insert(i, new std::string(slot_name));
    free((void *) slot_name);
  }
  catch (CommandFailedException &e){
    if (!e.reason_slot_not_programmed())
      throw;
    cache_HOTP_name.insert(i, new std::string(""));
  }
  return *cache_HOTP_name[i];
}

std::string libada::getPWSSlotName(const int i) {
  static QMutex mut;
  QMutexLocker locker(&mut);
  if (cache_PWS_name.contains(i)){
    return *cache_PWS_name[i];
  }
  try{
    const auto slot_name = nm::instance()->get_password_safe_slot_name(i);
    cache_PWS_name.insert(i, new std::string(slot_name));
    free((void *) slot_name);
  }
  catch (CommandFailedException &e){
    if (!e.reason_slot_not_programmed()) //FIXME correct reason
      throw;
    cache_PWS_name.insert(i, new std::string("(empty)"));
  }
  return *cache_PWS_name[i];
}

bool libada::getPWSSlotStatus(const int i) {
  if (status_PWS.empty()){
    status_PWS = nm::instance()->get_password_safe_slot_status();
  }
  return status_PWS[i];
}

void libada::erasePWSSlot(const int i) {
  nm::instance()->erase_password_safe_slot(i);
}

uint8_t libada::getStorageSDCardSizeGB() {
  return nm::instance()->get_SD_card_size();
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
  return true;
}

#include <QDebug>
#include "libnitrokey/include/DeviceCommunicationExceptions.h"
bool libada::isPasswordSafeUnlocked() {
  try{
    nm::instance()->get_password_safe_slot_status();
    return true;
  }
  catch (CommandFailedException &e){
    qDebug() << e.what();
    if (e.reason_not_authorized())
      return false;
    throw;
  }
  catch (DeviceCommunicationException &e){
    qDebug() << e.what();
    return false;
  }
}

bool libada::isTOTPSlotProgrammed(const int i) {
  return !getTOTPSlotName(i).empty();
}

bool libada::isHOTPSlotProgrammed(const int i) {
  return !getHOTPSlotName(i).empty();
}

void libada::writeToOTPSlot(const OTPSlot &otpconf, const QString &tempPassword) {
  bool res;
  const auto byteArray = tempPassword.toLatin1();
  switch(otpconf.type){
    case OTPSlot::OTPType::HOTP: {
      res = nm::instance()->write_HOTP_slot(otpconf.slotNumber, otpconf.slotName, otpconf.secret, otpconf.interval,
        otpconf.config_st.useEightDigits, otpconf.config_st.useEnter, otpconf.config_st.useTokenID,
      otpconf.tokenID, byteArray.constData());
    }
      break;
    case OTPSlot::OTPType::TOTP:
      res = nm::instance()->write_TOTP_slot(otpconf.slotNumber, otpconf.slotName, otpconf.secret, otpconf.interval,
                                otpconf.config_st.useEightDigits, otpconf.config_st.useEnter, otpconf.config_st.useTokenID,
                                otpconf.tokenID, byteArray.constData());
      break;
    case OTPSlot::UNKNOWN:
      throw std::runtime_error("invalid OTP data");
      break;
  }
}

bool libada::is_nkpro_07_rtm1() {
  return nm::instance()->get_connected_device_model() == nitrokey::DeviceModel::PRO
      && getMinorFirmwareVersion() == 7;
}

bool libada::is_secret320_supported() {
  return nm::instance()->is_320_OTP_secret_supported();
}

int libada::getTOTPCode(int i, const char *string) {
  return nm::instance()->get_TOTP_code(i, string);
}

int libada::getHOTPCode(int i, const char *string) {
  return nm::instance()->get_HOTP_code(i, string);
}

int libada::eraseHOTPSlot(const int i, const char *string) {
  return nm::instance()->erase_hotp_slot(i, string);
}

int libada::eraseTOTPSlot(const int i, const char *string) {
  return nm::instance()->erase_totp_slot(i, string);
}

#include <QDateTime>
bool libada::is_time_synchronized() {
  auto time = QDateTime::currentDateTimeUtc().toTime_t();
  try{
    nm::instance()->get_time(time);
    return true;
  }
  catch( CommandFailedException &e){
    if (!e.reason_timestamp_warning())
      throw;
    return false;
  }
  return false;
}

bool libada::set_current_time() {
  auto time = QDateTime::currentDateTimeUtc().toTime_t();
  nm::instance()->set_time(time);
  return true;
}

void libada::on_DeviceDisconnect() {
  //TODO imp perf compare serial numbers to not clear cache if it is the same device
  cache_TOTP_name.clear();
  cache_HOTP_name.clear();
  cache_PWS_name.clear();
  status_PWS.clear();
}
