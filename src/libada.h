//
// Created by sz on 09.01.17.
//

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

class libada : public QObject {
    Q_OBJECT
    Q_DISABLE_COPY(libada)
  private:
    static std::shared_ptr <libada> _instance;
    QCache<int, std::string> cache_TOTP_name;
    QCache<int, std::string> cache_HOTP_name;
    QCache<int, std::string> cache_PWS_name;
    std::vector <uint8_t> status_PWS;


public slots:
      void on_OTP_save(int slot_no, bool isHOTP);
      void on_PWS_save(int slot_no);
      void on_FactoryReset();
      void on_DeviceDisconnect();

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
    int getTOTPCode(int i, const char *string);
    int getHOTPCode(int i, const char *string);
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
    void writeToOTPSlot(const OTPSlot &otpconf, const QString &tempPassword);

    bool is_nkpro_07_rtm1();
    bool is_secret320_supported();

    bool communication_issues_flag = false;
};


#endif //NITROKEYAPP_LIBNITROKEY_ADAPTER_H
