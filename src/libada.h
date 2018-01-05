/*
 * Copyright (c) 2012-2018 Nitrokey UG
 *
 * This file is part of Nitrokey App.
 *
 * Nitrokey App is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey App is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#ifndef NITROKEYAPP_LIBNITROKEY_ADAPTER_H
#define NITROKEYAPP_LIBNITROKEY_ADAPTER_H

#include <memory>
#include <string>
#include "hotpslot.h"
#include <QString>

#define HOTP_SLOT_COUNT_MAX 3
#define TOTP_SLOT_COUNT_MAX 15
#define HOTP_SLOT_COUNT 3
#define TOTP_SLOT_COUNT 15

#define STICK10_PASSWORD_LEN 20
#define STICK20_PASSOWRD_LEN 20
#define CS20_MAX_UPDATE_PASSWORD_LEN 20

#define PWS_SLOT_COUNT 16
#define PWS_SLOTNAME_LENGTH 11
#define PWS_PASSWORD_LENGTH 20
#define PWS_LOGINNAME_LENGTH 32

#define DEBUG_STATUS_NO_DEBUGGING 0
#define DEBUG_STATUS_LOCAL_DEBUG 1
#define DEBUG_STATUS_DEBUG_ALL 2

#define STICK20_CMD_START_VALUE 0x20
#define STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS (STICK20_CMD_START_VALUE + 7)

#define MAX_HIDDEN_VOLUME_PASSOWORD_SIZE 20

#include <QObject>
#include <QCache>

#include "libnitrokey/include/stick10_commands.h"

#include <functional>
#include <QMutex>

class NameCache {
private:
  std::function<const char*(int)> getter;
  QCache<int, std::string> cache;
  QMutex mut;
public:
  NameCache(){};
  NameCache(const std::function<const char *(int)> &getter);
  ~NameCache();

  void setGetter(const std::function<const char *(int)> &getter);
  std::string getName(const int i);
  void remove(const int slot_no);
  void clear();
};

class libada : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(libada)
  private:
    static std::shared_ptr <libada> _instance;
    NameCache cache_TOTP_name;
    NameCache cache_HOTP_name;
    NameCache cache_PWS_name;

    std::vector <uint8_t> status_PWS;
    std::string cardSerial_cached;
  static constexpr int invalid_value = 99;
  std::atomic_int minor_firmware_version_cached {invalid_value} ;
  std::atomic_int secret320_supported_cached {invalid_value} ;


public slots:
      void on_OTP_save(int slot_no, bool isHOTP);
      void on_PWS_save(int slot_no);
      void on_FactoryReset();
      void on_DeviceDisconnect();

signals:
  void regenerateMenu();

public:
    explicit libada();
    ~libada();
    static std::shared_ptr<libada> i();

    int getMajorFirmwareVersion();
    int getMinorFirmwareVersion();
    int getAdminPasswordRetryCount();
    int getUserPasswordRetryCount();
    std::string getCardSerial();
    std::string getTOTPSlotName(const int i);
    std::string getHOTPSlotName(const int i);
    std::string getTOTPCode(int slot_number, const char *user_temporary_password);
    std::string getHOTPCode(int slot_number, const char *user_temporary_password);
    int eraseHOTPSlot(const int i, const char *string);
    int eraseTOTPSlot(const int i, const char *string);

    std::string getPWSSlotName(const int i);
    bool getPWSSlotStatus(const int i);
    void erasePWSSlot(const int i);

    uint8_t getStorageSDCardSizeGB();

    bool is_time_synchronized();
    bool set_current_time();

    bool isDeviceConnected()  const throw();
    bool isDeviceInitialized();
    bool isStorageDeviceConnected() const throw();
    bool isPasswordSafeAvailable();
    bool isPasswordSafeUnlocked();
    bool isTOTPSlotProgrammed(const int i);
    bool isHOTPSlotProgrammed(const int i);
    void writeToOTPSlot(const OTPSlot &otpconf, const char* tempPassword);

    bool is_nkpro_07_rtm1();
    bool is_secret320_supported();

  std::string get_serial_number();

  nitrokey::proto::stick10::GetStatus::ResponsePayload get_status();

  bool have_communication_issues_occurred();
};


#endif //NITROKEYAPP_LIBNITROKEY_ADAPTER_H
