/*
* Author: Copyright (C) Andrzej Surowiec 2012
*						Parts Rudolf Boeddeker  Date: 2013-08-13
*
* This file is part of GPF Crypto Stick.
*
* GPF Crypto Stick is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* GPF Crypto Stick is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GPF Crypto Stick. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DEVICE_H
#define DEVICE_H

#include <QtWidgets>

#include "hidapi.h"
#include "command.h"
#include "hotpslot.h"
#include "totpslot.h"
#include "stick20hid.h"

/***************************************************************

  Used usb device ids

***************************************************************/

#define VID_STICK_OTP 0x20A0
#define PID_STICK_OTP 0x4108

#define VID_STICK_20  0x20A0
#define PID_STICK_20  0x4109 // MSD + CCID + HID production id

#define VID_STICK_20_UPDATE_MODE  0x03EB
#define PID_STICK_20_UPDATE_MODE  0x2FF1


//#define PID_STICK_20    0x220D // MSD + CCID + HID test id
//#define PID_STICK_20    0x2309 // MSD + HID test id
//#define PID_STICK_20    0x220B // HID test id


class Response;

#define REPORT_SIZE 64
#define PAYLOAD_SIZE 53

#define CMD_GET_STATUS                 0x00
#define CMD_WRITE_TO_SLOT              0x01
#define CMD_READ_SLOT_NAME             0x02
#define CMD_READ_SLOT                  0x03
#define CMD_GET_CODE                   0x04
#define CMD_WRITE_CONFIG               0x05
#define CMD_ERASE_SLOT                 0x06
#define CMD_FIRST_AUTHENTICATE         0x07
#define CMD_AUTHORIZE                  0x08
#define CMD_GET_PASSWORD_RETRY_COUNT   0x09
#define CMD_CLEAR_WARNING              0x0A


#define DEBUG_STATUS_NO_DEBUGGING       0
#define DEBUG_STATUS_LOCAL_DEBUG        1
#define DEBUG_STATUS_DEBUG_ALL          2


#define STICK20_CMD_START_VALUE                         0x10
#define STICK20_CMD_ENABLE_CRYPTED_PARI                 (STICK20_CMD_START_VALUE +  0)
#define STICK20_CMD_DISABLE_CRYPTED_PARI                (STICK20_CMD_START_VALUE +  1)
#define STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI          (STICK20_CMD_START_VALUE +  2)
#define STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI         (STICK20_CMD_START_VALUE +  3)
#define STICK20_CMD_ENABLE_FIRMWARE_UPDATE              (STICK20_CMD_START_VALUE +  4)
#define STICK20_CMD_EXPORT_FIRMWARE_TO_FILE             (STICK20_CMD_START_VALUE +  5)
#define STICK20_CMD_GENERATE_NEW_KEYS                   (STICK20_CMD_START_VALUE +  6)
#define STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS      (STICK20_CMD_START_VALUE +  7)

#define STICK20_CMD_WRITE_STATUS_DATA            		(STICK20_CMD_START_VALUE +  8)
#define STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN      	(STICK20_CMD_START_VALUE +  9)
#define STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN     	(STICK20_CMD_START_VALUE + 10)

#define STICK20_CMD_SEND_PASSWORD_MATRIX                (STICK20_CMD_START_VALUE + 11)
#define STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA        (STICK20_CMD_START_VALUE + 12)
#define STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP          (STICK20_CMD_START_VALUE + 13)

#define STICK20_CMD_GET_DEVICE_STATUS                   (STICK20_CMD_START_VALUE + 14)
#define STICK20_CMD_SEND_DEVICE_STATUS                  (STICK20_CMD_START_VALUE + 15)

#define STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD         (STICK20_CMD_START_VALUE + 16)
#define STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP            (STICK20_CMD_START_VALUE + 17)
#define STICK20_CMD_SEND_PASSWORD                       (STICK20_CMD_START_VALUE + 18)
#define STICK20_CMD_SEND_NEW_PASSWORD                   (STICK20_CMD_START_VALUE + 19)
#define STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND             (STICK20_CMD_START_VALUE + 20)

#define STICK20_CMD_SEND_STARTUP                        (STICK20_CMD_START_VALUE + 21)
#define STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED (STICK20_CMD_START_VALUE + 22)
#define STICK20_CMD_SEND_LOCK_STICK_HARDWARE            (STICK20_CMD_START_VALUE + 23)

#define STICK20_CMD_PRODUCTION_TEST                     (STICK20_CMD_START_VALUE + 24)

#define STATUS_READY           0x00
#define STATUS_BUSY	           0x01
#define STATUS_ERROR           0x02
#define STATUS_RECEIVED_REPORT 0x03

#define CMD_STATUS_OK                  0
#define CMD_STATUS_WRONG_CRC           1
#define CMD_STATUS_WRONG_SLOT          2
#define CMD_STATUS_SLOT_NOT_PROGRAMMED 3
#define CMD_STATUS_WRONG_PASSWORD      4
#define CMD_STATUS_NOT_AUTHORIZED      5
#define CMD_STATUS_TIMESTAMP_WARNING   6

enum comm_errors{
    ERR_NOT_CONNECTED      = -1,
    ERR_WRONG_RESPONSE_CRC = -2,
    ERR_SENDING            = -3,
    ERR_STATUS_NOT_OK      = -4
};


#define STICK20_PASSOWRD_LEN               20
#define STICK20_PASSWORD_KIND_USER          0
#define STICK20_PASSWORD_KIND_ADMIN         1
#define STICK20_PASSWORD_KIND_RESET_USER    2


#define STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ALL_VOL              0
#define STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ENCRYPTED_VOL        1



#define OUTPUT_CMD_STICK20_STATUS_IDLE                      0
#define OUTPUT_CMD_STICK20_STATUS_OK                        1
#define OUTPUT_CMD_STICK20_STATUS_BUSY                      2
#define OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD            3
#define OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR          4
#define OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY     5
#define OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK   6
#define OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR           7


#define OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS  20

#define OUTPUT_CMD_STICK20_SEND_DATA_SIZE            25

#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_NONE        0
#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG       1
#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_PW_DATA     2
#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_STATUS      3


#define OUTPUT_CMD_STICK20_MATRIX_DATA_SIZE            25

typedef struct {
  unsigned char  Counter_u8;
  unsigned char  BlockNo_u8;
  unsigned char  SendSize_u8;
  unsigned char  MatrixData_u8[OUTPUT_CMD_STICK20_MATRIX_DATA_SIZE];
} HID_Stick20Matrix_est;


#define MAX_HIDDEN_VOLUME_PASSOWORD_SIZE  20

#pragma pack(push,1)

typedef struct {
    unsigned char SlotNr_u8;
    unsigned char StartBlockPercent_u8;
    unsigned char EndBlockPercent_u8;
    unsigned char HiddenVolumePassword_au8[MAX_HIDDEN_VOLUME_PASSOWORD_SIZE+1];
} HiddenVolumeSetup_tst;

#pragma pack(pop)


#ifdef _MSC_VER
    // For MSVC
    #define uint64_t unsigned long long
#endif

/*
#define HOTP_SLOT_COUNT      3
#define TOTP_SLOT_COUNT     15
*/
#define HOTP_SLOT_COUNT_MAX      3
#define TOTP_SLOT_COUNT_MAX     15

#define HOTP_SLOT_COUNT      3
#define TOTP_SLOT_COUNT      15

class Device
{



public:
    Device(int vid, int pid,int vidStick20, int pidStick20,int vidStick20UpdateMode, int pidStick20UpdateMode);
    hid_device *dev_hid_handle;
    int checkConnection();
    bool isConnected;
    int sendCommand(Command *cmd);
    int sendCommandGetResponse(Command *cmd, Response *resp);
    void connect();
    int getSlotName(uint8_t slotNo);
    int eraseSlot(uint8_t slotNo);
    int writeToHOTPSlot(HOTPSlot *slot);
    int writeToTOTPSlot(TOTPSlot *slot);
    int getCode(uint8_t slotNo, uint64_t challenge,uint64_t lastTOTPTime,uint8_t  lastInterval,uint8_t result[18]);
    int getHOTP(uint8_t slotNo);
    int readSlot(uint8_t slotNo);
    int getPasswordRetryCount();

    bool newConnection;
    void initializeConfig();
    HOTPSlot *HOTPSlots[HOTP_SLOT_COUNT_MAX];
    TOTPSlot *TOTPSlots[TOTP_SLOT_COUNT_MAX];
    void getSlotConfigs();
    uint8_t *password[25];
    bool validPassword;
    bool passwordSet;
    uint8_t passwordRetryCount;

    bool stick20EnableCryptedPartition (uint8_t *password);
    bool stick20DisableCryptedPartition (void);

    bool stick20EnableHiddenCryptedPartition (uint8_t *password);
    bool stick20DisableHiddenCryptedPartition (void);

    bool stick20EnableFirmwareUpdate (uint8_t *password);
    bool stick20ExportFirmware (uint8_t *password);

    bool stick20CreateNewKeys (uint8_t *password);
    bool stick20FillSDCardWithRandomChars (uint8_t *password,uint8_t VolumeFlag);

    bool stick20SetupHiddenVolume (void);
    bool stick20GetPasswordMatrix (void);
    bool stick20SendPasswordMatrixPinData (uint8_t *Pindata);
    bool stick20SendPasswordMatrixSetup (uint8_t *Setupdata);
    bool stick20GetStatusData ();
    int stick20SendPassword (uint8_t *Pindata);
    int stick20SendNewPassword (uint8_t *NewPindata);
    int stick20SendClearNewSdCardFound (uint8_t *Pindata);

    int stick20SendSetReadonlyToUncryptedVolume (uint8_t *Pindata);
    int stick20SendSetReadwriteToUncryptedVolume (uint8_t *Pindata);
    int stick20SendStartup (uint64_t localTime);
    int stick20SendHiddenVolumeSetup (HiddenVolumeSetup_tst *HV_Data_st);
    int stick20LockFirmware (uint8_t *password);
    int stick20ProductionTest (void);

    uint8_t cardSerial[4];
    uint8_t firmwareVersion[2];

    uint8_t generalConfig[3];

    uint8_t PasswordMatrix[100];
    uint8_t PasswordMatrixPinData[30];

    int getStatus();
    void getGeneralConfig();
    int writeGeneralConfig(uint8_t data[3]);

    int firstAuthenticate(uint8_t cardPassword[25],uint8_t tempPasswrod[25]);
    int authorize(Command *authorizedCmd);

    int  activStick20;
    bool waitForAckStick20;
    int  lastBlockNrStick20;

    int  HOTP_SlotCount;
    int  TOTP_SlotCount;



private:
    int vid;
    int pid;

    int vidStick20;
    int pidStick20;

    int vidStick20UpdateMode;
    int pidStick20UpdateMode;

};

#endif // DEVICE_H
