/*
 * Author: Copyright (C) Andrzej Surowiec 2012
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


#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>

#include "crc32.h"
#include "device.h"
#include "mcvs-wrapper.h"
#include "response.h"
#include "sleep.h"
#include "stick20-response-task.h"
#include "string.h"

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local defines

*******************************************************************************/

// #define LOCAL_DEBUG // activate for debugging

const int userPasswordRetryCount_notInitialized = 99;

/*******************************************************************************

  Device

  Constructor Device

  Changes
  Date      Author          Info
  25.03.14  RB              Dynamic slot counts

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

Device::Device(int vid, int pid, int vidStick20, int pidStick20, int vidStick20UpdateMode,
               int pidStick20UpdateMode) {
  needsReconnect = false;
  int i;

  LastStickError = OUTPUT_CMD_STICK20_STATUS_OK;

  dev_hid_handle = NULL;
  isConnected = false;

  this->vid = vid;
  this->pid = pid;

  validPassword = false;
  passwordSet = false;

  validUserPassword = false;

  memset(adminTemporaryPassword, 0, 25);
  memset(userTemporaryPassword, 0, 25);

  // Vars for password safe
  passwordSafeUnlocked = FALSE;
  passwordSafeAvailable = true;
  memset(passwordSafeStatusDisplayed, 0, PWS_SLOT_COUNT);

  for (i = 0; i < PWS_SLOT_COUNT; i++) {
    passwordSafeSlotNames[i][0] = 0;
  }

  // handle = hid_open(vid,pid, NULL);

  HOTP_SlotCount = HOTP_SLOT_COUNT; // For stick 1.0
  TOTP_SlotCount = TOTP_SLOT_COUNT;

  for (i = 0; i < HOTP_SLOT_COUNT_MAX; i++) {
    HOTPSlots[i] = new OTPSlot();
    HOTPSlots[i]->slotNumber = 0x10 + i;
  }

  for (i = 0; i < TOTP_SLOT_COUNT_MAX; i++) {
    TOTPSlots[i] = new OTPSlot();
    TOTPSlots[i]->slotNumber = 0x20 + i;
  }

  newConnection = true;

  // Init data for stick 20

  this->vidStick20 = vidStick20;
  this->pidStick20 = pidStick20;
  this->vidStick20UpdateMode = vidStick20UpdateMode;
  this->pidStick20UpdateMode = pidStick20UpdateMode;

  // this->vidStick20 = 0x046d;
  // this->pidStick20 = 0xc315;

  activStick20 = false;
  waitForAckStick20 = false;
  lastBlockNrStick20 = 0;
  passwordRetryCount = 0;

  this->userPasswordRetryCount = userPasswordRetryCount_notInitialized;
  this->passwordRetryCount = userPasswordRetryCount_notInitialized;
}

/*******************************************************************************

  checkConnection

  Check the presents of stick 1.0 or 2.0
  -1 disconnected, 0 connected, 1 just connected - needs initialization

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::checkConnection(int InitConfigFlag) {
  uint8_t buf[65] = {0};
  static int DisconnectCounter = 0;
  int res;

  if (!dev_hid_handle) {
    isConnected = false;
    newConnection = true;
    DisconnectCounter = 0;
    return -1;
  } else {
    static int Counter = 0;
    Counter++;

    res = hid_get_feature_report(dev_hid_handle, buf, 65);
    if (res < 0) {
      DisconnectCounter++;
      if (1 < DisconnectCounter) {
        isConnected = false;
        newConnection = true;
        return -1;
      }
    } else {
      DisconnectCounter = 0;
    }

    if (newConnection) {
      isConnected = true;
      newConnection = false;
      passwordSet = false;
      validPassword = false;
      validUserPassword = false;

      HOTP_SlotCount = HOTP_SLOT_COUNT; // For stick 1.0
      TOTP_SlotCount = TOTP_SLOT_COUNT;

      passwordSafeUnlocked = FALSE;

      // stick 20 with no OTP
      if (TRUE == activStick20) {
        HOTP_SlotCount = HOTP_SLOT_COUNT_MAX;
        TOTP_SlotCount = TOTP_SLOT_COUNT_MAX;
        passwordSafeUnlocked = FALSE;
      }
      if (TRUE == InitConfigFlag) {
        initializeConfig(); // init data for OTP
      }
      return 1;
    }

    return 0;
  }
}

/*******************************************************************************

  connect

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Device::disconnect() {  
  activStick20 = false;
  if (NULL != dev_hid_handle) {
    hid_close(dev_hid_handle);
    dev_hid_handle = NULL;
    hid_exit();
  }
  this->userPasswordRetryCount = userPasswordRetryCount_notInitialized;
  this->passwordRetryCount = userPasswordRetryCount_notInitialized;
}

void Device::connect() {
  // Disable stick 20
  activStick20 = false;

  dev_hid_handle = hid_open(vid, pid, NULL);

  // Check for stick 20
  if (NULL == dev_hid_handle) {
    dev_hid_handle = hid_open(vidStick20, pidStick20, NULL);
    if (NULL != dev_hid_handle) {
      // Stick 20 found
      activStick20 = true;
    }
  }
}

/*******************************************************************************
  sendCommand
  Send a command to the stick via the send feature report HID message
  Report size is 64 byte

  Byte  0       = 0
  Byte  1       = cmd type
  Byte  2-60    = payload
  Byte 61-64    = CRC 32 from cmd type and payload = 15 long words - 60 bytes
*******************************************************************************/

int Device::sendCommand(Command *cmd) {
  uint8_t report[REPORT_SIZE + 1] = {0};
  int err;

  report[1] = cmd->commandType;

  size_t len = std::min(sizeof(report) - 2, sizeof(cmd->data));
  memcpy(report + 2, cmd->data, len);
    cmd->generateCRC();
  ((uint32_t *)(report + 1))[15] = cmd->crc;

  if (0 == dev_hid_handle) {
    return (-1); // Return error
  }
  if (!activStick20 &&
      cmd->commandType != CMD_GET_USER_PASSWORD_RETRY_COUNT &&
      cmd->commandType != CMD_GET_PASSWORD_RETRY_COUNT &&
      cmd->commandType != CMD_GET_STATUS &&
          !isInitialized()) {
    DebugAppendTextGui(QString("Device not initialized. Skipping command %1.\n")
                           .arg(cmd->commandType).toLatin1().data() );
    return (-1); // Return error
  }
  err = hid_send_feature_report(dev_hid_handle, report, sizeof(report));

  {
    char text[1000];

    int i;

    static int Counter = 0;

    if (STICK20_CMD_SEND_DEBUG_DATA != report[1]) // Log no debug infos
    {
      SNPRINTF(text, sizeof(text), "%6d :sendCommand0: ", Counter);

      Counter++;
      DebugAppendTextGui(text);
      for (i = 0; i <= 64; i++) {
        SNPRINTF(text, sizeof(text), "%02x ", (unsigned char)report[i]);
        DebugAppendTextGui(text);
      }
      SNPRINTF(text, sizeof(text), "\n");

      DebugAppendTextGui(text);
    }
  }

  return err;
}

bool Device::isInitialized() const { return
            !needsReconnect &&
            userPasswordRetryCount != userPasswordRetryCount_notInitialized
            && HID_Stick20Configuration_st.UserPwRetryCount != userPasswordRetryCount_notInitialized
            && HID_Stick20Configuration_st.AdminPwRetryCount != userPasswordRetryCount_notInitialized
            ; }

int Device::sendCommandGetResponse(Command *cmd, Response *resp) {
  uint8_t report[REPORT_SIZE + 1];

  int i;

  int err;

  if (!isConnected)
    return ERR_NOT_CONNECTED;

  memset(report, 0, sizeof(report));
  report[1] = cmd->commandType;

  size_t len = std::min(sizeof(report) - 2, sizeof(cmd->data));
  memcpy(report + 2, cmd->data, len);

  uint32_t crc = 0xffffffff;

  for (i = 0; i < 15; i++) {
    crc = Crc32(crc, ((uint32_t *)(report + 1))[i]);
  }
  ((uint32_t *)(report + 1))[15] = crc;

  cmd->crc = crc;

  err = hid_send_feature_report(dev_hid_handle, report, sizeof(report));

  {
    char text[1000];

    int i;

    static int Counter = 0;

    if (STICK20_CMD_SEND_DEBUG_DATA != report[1]) // Log no debug infos
    {
      SNPRINTF(text, sizeof(text), "%6d :sendCommand1: ", Counter);
      Counter++;
      DebugAppendTextGui(text);
      for (i = 0; i <= 64; i++) {
        SNPRINTF(text, sizeof(text), "%02x ", (unsigned char)report[i]);
        DebugAppendTextGui(text);
      }
      SNPRINTF(text, sizeof(text), "\n");
      DebugAppendTextGui(text);
    }
  }

  if (err == -1)
    return ERR_SENDING;

  Sleep::msleep(100);

  // Response *resp=new Response();
  resp->getResponse(this);

  if (cmd->crc != resp->lastCommandCRC)
    return ERR_WRONG_RESPONSE_CRC;

  if (resp->lastCommandStatus == CMD_STATUS_OK)
    return 0;

  return ERR_STATUS_NOT_OK;
}

/*******************************************************************************

  getSlotName

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::getSlotName(uint8_t slotNo) {
  int res;

  uint8_t data[1];

  data[0] = slotNo;

  if (isConnected) {
    Command *cmd = new Command(CMD_READ_SLOT_NAME, data, 1);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return -1;
    } else { // sending the command was successful
      // return cmd->crc;
      Sleep::msleep(100);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) { // the response was for the last command
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          if ((slotNo >= 0x10) && (slotNo < 0x10 + HOTP_SlotCount)) {
            memcpy(HOTPSlots[slotNo & 0x0F]->slotName, resp->data, 15);
            HOTPSlots[slotNo & 0x0F]->isProgrammed = true;
          } else if ((slotNo >= 0x20) && (slotNo < 0x20 + TOTP_SlotCount)) {
            memcpy(TOTPSlots[slotNo & 0x0F]->slotName, resp->data, 15);
            TOTPSlots[slotNo & 0x0F]->isProgrammed = true;
          }

        } else if (resp->lastCommandStatus == CMD_STATUS_SLOT_NOT_PROGRAMMED) {
          if ((slotNo >= 0x10) && (slotNo < 0x10 + HOTP_SlotCount)) {
            HOTPSlots[slotNo & 0x0F]->isProgrammed = false;
            HOTPSlots[slotNo & 0x0F]->slotName[0] = 0;
          } else if ((slotNo >= 0x20) && (slotNo < 0x20 + TOTP_SlotCount)) {
            TOTPSlots[slotNo & 0x0F]->isProgrammed = false;
            TOTPSlots[slotNo & 0x0F]->slotName[0] = 0;
          }
        }
      }
      delete cmd;

      return 0;
    }
  }
  return -1;
}

/*******************************************************************************

  eraseSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::eraseSlot(uint8_t slotNo) {
  int res;
  uint8_t data[1];
  data[0] = slotNo;

  if (!isConnected) {
    return ERR_NOT_CONNECTED; // communication error
  }

  const auto data_with_password_len = sizeof(data) + sizeof(adminTemporaryPassword);
  uint8_t data_with_password[data_with_password_len] = {};
  memcpy(data_with_password, data, sizeof(data));
  memcpy(data_with_password + sizeof(data), adminTemporaryPassword, sizeof(adminTemporaryPassword));

  Command cmd(CMD_ERASE_SLOT, data_with_password, sizeof(data_with_password));
  authorize(&cmd);
  res = sendCommand(&cmd);

  if (res == -1) {
    return ERR_SENDING; // communication error
  }
  Sleep::msleep(200);

  Response resp;
  resp.getResponse(this);
  if (cmd.crc == resp.lastCommandCRC) {
    return resp.lastCommandStatus;
  }
  return ERR_WRONG_RESPONSE_CRC; // wrong crc
}

/*******************************************************************************

  setTime

  reset  =  0   Get time
            1   Set time

  Reviews
  Date      Reviewer        Info
  27.07.14  SN              First review

*******************************************************************************/

int Device::setTime(int reset) {
  int res;

  uint8_t data[30];

  uint64_t time = QDateTime::currentDateTime().toTime_t();

  memset(data, 0, 30);
  data[0] = reset;
  memcpy(data + 1, &time, 8);

  if (isConnected) {
    Command *cmd = new Command(CMD_SET_TIME, data, 9);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return -1;
    } else { // sending the command was successful
      Sleep::msleep(100);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) { // the response was for the last command
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
        } else if (resp->lastCommandStatus == CMD_STATUS_TIMESTAMP_WARNING) {
          delete cmd;

          return -2;
        }
      }
      delete cmd;

      return 0;
    }
  }
  return -1;
}

/*******************************************************************************

  writeToHOTPSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

#define __packed __attribute__((__packed__))


struct WriteToOTPSlot {
    uint8_t temporary_admin_password[25];
    uint8_t slot_number;
    union {
        uint64_t slot_counter_or_interval;
        uint8_t slot_counter_s[8];
    } __packed;
    uint8_t _slot_config;
    uint8_t slot_token_id[13]; /** OATH Token Identifier */
} __packed;

struct SendOTPData {
    uint8_t temporary_admin_password[25];
    uint8_t type; //S-secret, N-name
    uint8_t id; //multiple reports for values longer than 30 bytes
    uint8_t data[30]; //data, does not need null termination
} __packed;

int Device::writeToOTPSlot(OTPSlot *slot) {
  if (!isConnected) {
    return ERR_NOT_CONNECTED; // other issue
  }

  const auto slotNumber = slot->slotNumber;
  if (!is_HOTP_slot_number(slotNumber) && !is_TOTP_slot_number(slotNumber)) {
    return -1; // wrong slot number checked on app side //TODO ret code conflict
  }
  int res;
  uint8_t data[COMMAND_SIZE];
  WriteToOTPSlot write_data;
  int buffer_size = 0;
  uint8_t *buffer = nullptr;

  if (!is_auth08_supported()){
    buffer = data;
    buffer_size = sizeof(data);
    memset(data, 0, sizeof(data));
    data[0] = slotNumber;
    memcpy(data + 1, slot->slotName, 15);
    memcpy(data + 16, slot->secret, 20);
    data[36] = slot->config;
    memcpy(data + 37, slot->tokenID, 13);
    if (!is_HOTP_slot_number(slotNumber)) {
      memcpy(data + 50, slot->counter, 8);
    } else if (is_TOTP_slot_number(slotNumber)) {
      memcpy(data + 50, &slot->interval, 2);
    }
  } else {
    //copy other OTP data
    //name
    SendOTPData otpData;
    memset(&otpData, 0, sizeof(otpData));
    otpData.id = 0;
    otpData.type = 'N';
    memcpy(otpData.data, slot->slotName, sizeof(slot->slotName));
    memcpy(otpData.temporary_admin_password, adminTemporaryPassword, sizeof(adminTemporaryPassword));
    //execute command
    Command cmd_otp_name(CMD_SEND_OTP_DATA, (uint8_t *) &otpData, sizeof(otpData));
    res = sendCommand(&cmd_otp_name);
    if (res == -1) return ERR_SENDING; // communication error
    Response resp;
    resp.getResponse(this);

    //secret
    otpData.type = 'S';
    otpData.id = 0;

    const auto secret_size = sizeof(slot->secret);
    auto remaining_secret_length = secret_size;

    while (remaining_secret_length>0){
      const auto bytesToCopy = std::min(sizeof(otpData.data), remaining_secret_length);
      const auto start = secret_size - remaining_secret_length;
      memset(otpData.data, 0, sizeof(otpData.data));
      memcpy(otpData.data, slot->secret + start, bytesToCopy);
      //execute command
      Command cmd_otp_secret(CMD_SEND_OTP_DATA, (uint8_t *) &otpData, sizeof(otpData));
      res = sendCommand(&cmd_otp_secret);
      if (res == -1) return ERR_SENDING; // communication error
      Response resp;
      resp.getResponse(this);

      remaining_secret_length -= bytesToCopy;
      otpData.id++;
    }

    //write OTP slot data
    buffer = (uint8_t*) &write_data;
    buffer_size = sizeof(write_data);
    memcpy(write_data.temporary_admin_password, adminTemporaryPassword, sizeof(adminTemporaryPassword));
    write_data.slot_number = slotNumber;
    if(is_HOTP_slot_number(slotNumber)){
      memcpy(write_data.slot_counter_s, slot->counter, sizeof(slot->counter));
    } else if (is_TOTP_slot_number(slotNumber)){
      write_data.slot_counter_or_interval = slot->interval;
    }

    write_data._slot_config = slot->config;
    memcpy(write_data.slot_token_id, slot->tokenID, sizeof(slot->tokenID));
  }

  Command cmd(CMD_WRITE_TO_SLOT, buffer, buffer_size);
  authorize(&cmd);
  res = sendCommand(&cmd);
  if (res == -1) return ERR_SENDING; // communication error

  Sleep::msleep(100);
  Response resp;
  resp.getResponse(this);

  if (cmd.crc != resp.lastCommandCRC) {
    return ERR_WRONG_RESPONSE_CRC; // wrong crc
  }
  return resp.lastCommandStatus;
}

bool Device::is_HOTP_slot_number(const uint8_t slotNumber) const {
  return ((slotNumber >= 0x10) && (slotNumber < 0x10 + HOTP_SlotCount));
}

bool Device::is_secret320_supported() const {
  return is_nk_pro() && get_major_firmware_version() >= 8;
      //|| is_nk_storage() && get_major_firmware_version() >= 44;
}

bool Device::is_auth08_supported() const {
  return is_nk_pro() && get_major_firmware_version() >= 8;
      //|| is_nk_storage() && get_major_firmware_version() >= 44;
}

bool Device::is_TOTP_slot_number(const uint8_t slotNumber) const {
  return (slotNumber >= 0x20) && (slotNumber < 0x20 + TOTP_SlotCount);
}

/*******************************************************************************

  getCode

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::getCode(uint8_t slotNo, uint64_t challenge, uint64_t lastTOTPTime, uint8_t lastInterval,
                    uint8_t result[18]) {
  bool is_OTP_PIN_protected = otpPasswordConfig[0] == 1;

  uint8_t data[18];
  data[0] = slotNo;
  memcpy(data + 1, &challenge, 8);
  // Time of challenge: Warning:
  // it's better to tranfer time
  // and interval, to avoid attacks
  // with wrong timestamps
  memcpy(data + 9, &lastTOTPTime, 8);
  memcpy(data + 17, &lastInterval, 1);

  if (isConnected) {
    const auto data_with_password_len = sizeof(data) + sizeof(userTemporaryPassword);
    uint8_t data_with_password[data_with_password_len] = {};
    memcpy(data_with_password, data, sizeof(data));
    memcpy(data_with_password + sizeof(data), userTemporaryPassword, sizeof(userTemporaryPassword));

    Command cmd(CMD_GET_CODE, data_with_password, sizeof(data_with_password));

    if (is_OTP_PIN_protected) {
      userAuthorize(&cmd);
    }

    int res = sendCommand(&cmd);
    if (res == -1) {
      return ERR_SENDING; // connection problem
    }
    Sleep::msleep(100);

    Response resp;
    resp.getResponse(this);
    if (cmd.crc == resp.lastCommandCRC) { // the response was for the last command
      if (resp.lastCommandStatus == CMD_STATUS_OK) {
        memcpy(result, resp.data, 18);
        return ERR_NO_ERROR; // OK!
      }
    }
    return ERR_WRONG_RESPONSE_CRC; // wrong CRC or not OK status
  }
  return ERR_NOT_CONNECTED; // connection problem
}

int Device::getHOTP(uint8_t slotNo) {
  int res;

  uint8_t data[9];

  data[0] = slotNo;
  // memcpy(data+1,&challenge,8);

  Command *cmd = new Command(CMD_GET_CODE, data, 9);

  Response *resp = new Response();

  res = sendCommandGetResponse(cmd, resp);

  delete cmd;

  return res;
}

/*******************************************************************************

  readSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/
int Device::readSlot(uint8_t slotNo) {
  int res;

  uint8_t data[1];
  data[0] = slotNo;

  if (isConnected) {
    Command cmd(CMD_READ_SLOT, data, 1);

    res = sendCommand(&cmd);

    if (res == -1) {
      return -1;
    } else { // sending the command was successful
      Sleep::msleep(100);
      Response resp;

      resp.getResponse(this);

      if (cmd.crc == resp.lastCommandCRC) { // the response was for the last command
          const int s = slotNo & 0x0F;
          if (resp.lastCommandStatus == CMD_STATUS_OK) {
          if ((slotNo >= 0x10) && (slotNo < 0x10 + HOTP_SlotCount)) {
            memcpy(HOTPSlots[s]->slotName, resp.data, 15);
            HOTPSlots[s]->config = (uint8_t)resp.data[15];
            memcpy(HOTPSlots[s]->tokenID, resp.data + 16, 13);
            memcpy(HOTPSlots[s]->counter, resp.data + 29, 8);
            HOTPSlots[s]->isProgrammed = true;
          } else if ((slotNo >= 0x20) && (slotNo < 0x20 + TOTP_SlotCount)) {
            memcpy(TOTPSlots[s]->slotName, resp.data, 15);
            TOTPSlots[s]->config = (uint8_t)resp.data[15];
            memcpy(TOTPSlots[s]->tokenID, resp.data + 16, 13);
            memcpy(&(TOTPSlots[s]->interval), resp.data + 29, 2);
            TOTPSlots[s]->isProgrammed = true;
          }

        } else if (resp.lastCommandStatus == CMD_STATUS_SLOT_NOT_PROGRAMMED) {
          if ((slotNo >= 0x10) && (slotNo < 0x10 + HOTP_SlotCount)) {
            HOTPSlots[s]->isProgrammed = false;
            HOTPSlots[s]->slotName[0] = 0;
          } else if ((slotNo >= 0x20) && (slotNo < 0x20 + TOTP_SlotCount)) {
            TOTPSlots[s]->isProgrammed = false;
            TOTPSlots[s]->slotName[0] = 0;
          }
        }
      }
      return 0;
    }
  }
  return -1;
}

/*******************************************************************************

  initializeConfig

  Changes
  Date      Author          Info
  25.03.14  RB              Slot count by define

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Device::initializeConfig() {
  int i;
  unsigned int currentTime;
  getStatus();
  getUserPasswordRetryCount();
  getPasswordRetryCount();

  for (i = 0; i < HOTP_SlotCount; i++) {
    readSlot(0x10 + i);
  }

  for (i = 0; i < TOTP_SlotCount; i++) {
    readSlot(0x20 + i);
  }

  if (TRUE == activStick20) {
    currentTime = QDateTime::currentDateTime().toTime_t();
    stick20SendStartup((unsigned long long)currentTime);
  }
}

/*******************************************************************************

  getSlotConfigs

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Device::getSlotConfigs() {
  /* Removed from firmware readSlot(0x10); readSlot(0x11); readSlot(0x20); readSlot(0x21);
   * readSlot(0x22); readSlot(0x23); */
}

/*******************************************************************************

  getStatus

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::getStatus() {
  bool correctCRC=false;  
  QString logMessage;

  if (!isConnected) {
    return -2; // device not connected
  }

  Command cmd(CMD_GET_STATUS, Q_NULLPTR, 0);
  int res = sendCommand(&cmd);

  if (res != -1) {
  // sending the command was successful
  Sleep::msleep(100);
  Response resp;
  resp.getResponse(this);

  correctCRC = cmd.crc == resp.lastCommandCRC;
    if (correctCRC) {
        int responseCode = 0;
      if(needsReconnect){
          needsReconnect = false;
          logMessage = QString(__FUNCTION__) + ": Reconnecting device\n";
          DebugAppendTextGui(logMessage.toLatin1().data());
          this->disconnect();
          responseCode = 1;
      }
    memcpy(firmwareVersion, resp.data, 2);
    memcpy(cardSerial, resp.data + 2, 4);
    memcpy(generalConfig, resp.data + 6, 3);
    memcpy(otpPasswordConfig, resp.data + 9, 2);
    return responseCode; //OK
  }
}

  const int maxTries = 5;
  const int maxTriesToReconnection = 2 * maxTries;
  static int connectionProblemCounter = 0;

  ++connectionProblemCounter;
  if (res != -1 && !correctCRC) {
    logMessage = QString(__FUNCTION__) +
                 QString(": CRC other than expected %1/%2\n").arg(connectionProblemCounter).arg(maxTries);
    DebugAppendTextGui(logMessage.toLatin1().data());
  }
  if (res == -1) {
    logMessage = QString(__FUNCTION__) +
                 QString(": Other communication problem %1/%2\n").arg(connectionProblemCounter).arg(maxTries);
    DebugAppendTextGui(logMessage.toLatin1().data());
  }
  if (connectionProblemCounter % maxTries == 0) {
    if (connectionProblemCounter > maxTriesToReconnection) {
      connectionProblemCounter = 0;
      return -11; // fatal error, cannot resume communication, ask user for reinsertion
    }
    needsReconnect = true;
    return -10; // problems with communication, received CRC other than expected, try to reinitialize
  }

  return -100; //another error, should not be reached
}

/*******************************************************************************

  getPasswordRetryCount

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordRetryCount() {
  int res;

  uint8_t data[1];

  if (isConnected) {
    Command *cmd = new Command(CMD_GET_PASSWORD_RETRY_COUNT, data, 0);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(1000);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        passwordRetryCount = resp->data[0];
        HID_Stick20Configuration_st.AdminPwRetryCount = passwordRetryCount;
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
    delete cmd;
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getUserPasswordRetryCount

  Changes
  Date      Reviewer        Info
  10.08.14  SN              Function created

*******************************************************************************/

int Device::getUserPasswordRetryCount() {
  int res;

  if (isConnected) {
    Command cmd (CMD_GET_USER_PASSWORD_RETRY_COUNT, Q_NULLPTR, 0);

    res = sendCommand(&cmd);

    if (res == -1) {
      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(1000);
      Response resp;

      resp.getResponse(this);

      if (cmd.crc == resp.lastCommandCRC) {
        userPasswordRetryCount = resp.data[0];
        HID_Stick20Configuration_st.UserPwRetryCount = userPasswordRetryCount;
      } else {
        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getPasswordSafeSlotStatus

  Changes
  Date      Author        Info
  29.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordSafeSlotStatus() {
  int res;

  uint8_t data[1];

  // Clear entries
  memset(passwordSafeStatus, 0, PWS_SLOT_COUNT);

  if (isConnected) {
    Command *cmd = new Command(CMD_GET_PW_SAFE_SLOT_STATUS, data, 0);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(500);

      Response *resp = new Response();

      resp->getResponse(this);
      LastStickError = resp->deviceStatus;

      if (cmd->crc == resp->lastCommandCRC) {
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          memcpy(passwordSafeStatus, &resp->data[0], PWS_SLOT_COUNT);
          delete cmd;

          return (ERR_NO_ERROR);
        } else {
          delete cmd;

          return (ERR_STATUS_NOT_OK);
        }

      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getPasswordSafeSlotName

  Changes
  Date      Author        Info
  29.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordSafeSlotName(int Slot) {
  int res;

  uint8_t data[1];

  data[0] = Slot;

  if (isConnected) {
    Command *cmd = new Command(CMD_GET_PW_SAFE_SLOT_NAME, data, 1);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(200);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        memcpy(passwordSafeSlotName, &resp->data[0], PWS_SLOTNAME_LENGTH);
        passwordSafeSlotName[PWS_SLOTNAME_LENGTH] = 0;
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          delete cmd;

          return (ERR_NO_ERROR);
        } else {
          delete cmd;

          return (ERR_STATUS_NOT_OK);
        }
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getPasswordSafeSlotPassword

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordSafeSlotPassword(int Slot) {
  int res;

  uint8_t data[1];

  data[0] = Slot;

  if (isConnected) {
    Command *cmd = new Command(CMD_GET_PW_SAFE_SLOT_PASSWORD, data, 1);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(200);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        memcpy(passwordSafePassword, &resp->data[0], PWS_PASSWORD_LENGTH);
        passwordSafePassword[PWS_PASSWORD_LENGTH] = 0;

        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          delete cmd;

          return (ERR_NO_ERROR);
        } else {
          delete cmd;

          return (ERR_STATUS_NOT_OK);
        }
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getPasswordSafeSlotLoginName

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordSafeSlotLoginName(int Slot) {
  int res;

  uint8_t data[1];

  data[0] = Slot;

  if (isConnected) {
    Command *cmd = new Command(CMD_GET_PW_SAFE_SLOT_LOGINNAME, data, 1);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(200);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        memcpy(passwordSafeLoginName, &resp->data[0], PWS_LOGINNAME_LENGTH);
        passwordSafeLoginName[PWS_LOGINNAME_LENGTH] = 0;
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          delete cmd;

          return (ERR_NO_ERROR);
        } else {
          delete cmd;

          return (ERR_STATUS_NOT_OK);
        }
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  setPasswordSafeSlotData_1

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::setPasswordSafeSlotData_1(int Slot, uint8_t *Name, uint8_t *Password) {
  int res;

  uint8_t data[1 + PWS_SLOTNAME_LENGTH + PWS_PASSWORD_LENGTH + 1];

  data[0] = Slot;
  memcpy(&data[1], Name, PWS_SLOTNAME_LENGTH);
  memcpy(&data[1 + PWS_SLOTNAME_LENGTH], Password, PWS_PASSWORD_LENGTH);

  if (isConnected) {
    Command *cmd = new Command(CMD_SET_PW_SAFE_SLOT_DATA_1, data,
                               1 + PWS_SLOTNAME_LENGTH + PWS_PASSWORD_LENGTH);
    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(200);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        delete cmd;

        return ERR_NO_ERROR;
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  setPasswordSafeSlotData_2

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::setPasswordSafeSlotData_2(int Slot, uint8_t *LoginName) {
  int res;

  uint8_t data[1 + PWS_LOGINNAME_LENGTH + 1];

  data[0] = Slot;
  memcpy(&data[1], LoginName, PWS_LOGINNAME_LENGTH);

  if (isConnected) {
    Command *cmd = new Command(CMD_SET_PW_SAFE_SLOT_DATA_2, data, 1 + PWS_LOGINNAME_LENGTH);
    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(400);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        delete cmd;

        return ERR_NO_ERROR;
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  passwordSafeEraseSlot

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::passwordSafeEraseSlot(int Slot) {
  int res;

  uint8_t data[1];

  data[0] = Slot;

  if (isConnected) {
    Command *cmd = new Command(CMD_PW_SAFE_ERASE_SLOT, data, 1);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(500);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        delete cmd;

        return resp->lastCommandStatus;
        /* dead code if (resp->lastCommandStatus == CMD_STATUS_OK) { return (ERR_NO_ERROR); } else {
         * return (ERR_STATUS_NOT_OK); } */
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  passwordSafeEnable

  Changes
  Date      Author        Info
  01.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::passwordSafeEnable(char *password) {
  int res;

  uint8_t data[50];

  STRNCPY((char *)data, sizeof(data), password, 30);

  data[30 + 1] = 0;

  if (isConnected) {
    Command *cmd = new Command(CMD_PW_SAFE_ENABLE, data, strlen((char *)data));
    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(1500);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {

        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          passwordSafeUnlocked = TRUE;
          HID_Stick20Configuration_st.UserPwRetryCount = 3;
          delete cmd;

          return (ERR_NO_ERROR);
        } else {
          if (0 < HID_Stick20Configuration_st.UserPwRetryCount) {
            HID_Stick20Configuration_st.UserPwRetryCount--;
          }
          delete cmd;

          return resp->lastCommandStatus;
          // return (ERR_STATUS_NOT_OK);
        }
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  passwordSafeInitKey

  Changes
  Date      Author        Info
  01.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::passwordSafeInitKey(void) {
  int res;

  uint8_t data[1];

  data[0] = 0;

  if (isConnected) {
    Command *cmd = new Command(CMD_PW_SAFE_INIT_KEY, data, 1);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(500);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          delete cmd;

          return (ERR_NO_ERROR);
        } else {
          delete cmd;

          return (ERR_STATUS_NOT_OK);
        }
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  passwordSafeSendSlotDataViaHID

  Changes
  Date      Author        Info
  01.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::passwordSafeSendSlotDataViaHID(int Slot, int Kind) {
  int res;

  uint8_t data[2];

  data[0] = Slot;
  data[1] = Kind;

  if (isConnected) {
    Command *cmd = new Command(CMD_PW_SAFE_SEND_DATA, data, 2);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else { // sending the command was successful
      Sleep::msleep(200);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          delete cmd;

          return (ERR_NO_ERROR);
        } else {
          delete cmd;

          return (ERR_STATUS_NOT_OK);
        }
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getHighwaterMarkFromSdCard

  Get the higest accessed SD blocks in % of SD size
  (Only sice last poweron)

  Changes
  Date      Author        Info
  09.10.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

#define DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MIN 0
#define DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MAX 1
#define DEVICE_HIGH_WATERMARK_SD_CARD_READ_MIN 2
#define DEVICE_HIGH_WATERMARK_SD_CARD_READ_MAX 3

int Device::getHighwaterMarkFromSdCard(unsigned char *WriteLevelMin, unsigned char *WriteLevelMax,
                                       unsigned char *ReadLevelMin, unsigned char *ReadLevelMax) {
  int res;

  uint8_t data[1];

  if (isConnected) {
    Command *cmd = new Command(CMD_SD_CARD_HIGH_WATERMARK, data, 0);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return (ERR_SENDING);
    } else { // sending the command was successful
      Sleep::msleep(200);

      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        *WriteLevelMin = resp->data[DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MIN];
        *WriteLevelMax = resp->data[DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MAX];
        *ReadLevelMin = resp->data[DEVICE_HIGH_WATERMARK_SD_CARD_READ_MIN];
        *ReadLevelMax = resp->data[DEVICE_HIGH_WATERMARK_SD_CARD_READ_MAX];

        delete cmd;

        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          return (ERR_NO_ERROR);
        } else {
          return (ERR_STATUS_NOT_OK);
        }
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return (ERR_NOT_CONNECTED);
}

/*******************************************************************************

  getGeneralConfig

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Device::getGeneralConfig() {}

/*******************************************************************************

  writeGeneralConfig

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::writeGeneralConfig(uint8_t data[]) {
  int res;

  if (!isConnected) {
    return ERR_NOT_CONNECTED;
  }

  const auto data_with_password_len = 5 + sizeof(adminTemporaryPassword);
  uint8_t data_with_password[data_with_password_len] = {};
  memcpy(data_with_password, data, 5);
  memcpy(data_with_password + 5, adminTemporaryPassword, sizeof(adminTemporaryPassword));

  Command cmd(CMD_WRITE_CONFIG, data_with_password, sizeof(data_with_password));
  authorize(&cmd);
  res = sendCommand(&cmd);

  if (res == -1) {
    return ERR_SENDING;
  }
  // sending the command was successful
  Sleep::msleep(100);
  Response resp;

  resp.getResponse(this);

  if (cmd.crc != resp.lastCommandCRC) {
    return ERR_WRONG_RESPONSE_CRC;
  }
  switch (resp.lastCommandStatus) {
    case CMD_STATUS_OK:
    case CMD_STATUS_NOT_AUTHORIZED:
      return resp.lastCommandStatus;
    default:
      break;
  }
  return ERR_STATUS_NOT_OK;
}

/*******************************************************************************

  firstAuthenticate

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::firstAuthenticate(uint8_t cardPassword[], uint8_t tempPasswrod[]) {
  int res;

  uint8_t data[50] = {0};

  uint32_t crc;

  strncpy((char *)data, (const char *)cardPassword, 25);
  memcpy(data + 25, tempPasswrod, 25);

  if (isConnected) {
    Command *cmd = new Command(CMD_FIRST_AUTHENTICATE, data, 50);

    res = sendCommand(cmd);
    crc = cmd->crc;

    // remove the card password from memory
    delete cmd;

    memset(data, 0, sizeof(data));

    if (res == -1) {
      return -1;
    } else { // sending the command was successful
      Sleep::msleep(1000);
      Response *resp = new Response();

      resp->getResponse(this);

      if (crc == resp->lastCommandCRC) { // the response was for the last command
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          memcpy(adminTemporaryPassword, tempPasswrod, 25);
          validPassword = true;
          HID_Stick20Configuration_st.AdminPwRetryCount = 3;
          return 0;
        } else if (resp->lastCommandStatus == CMD_STATUS_WRONG_PASSWORD) {
          if (0 < HID_Stick20Configuration_st.AdminPwRetryCount) {
            HID_Stick20Configuration_st.AdminPwRetryCount--;
          }
          return -3;
        }
      }
    }
  }
  return -2;
}

/*******************************************************************************

  userAuthenticate

  Reviews
  Date      Reviewer        Info
  10.08.14  SN              First review

*******************************************************************************/

int Device::userAuthenticate(uint8_t cardPassword[], uint8_t tempPassword[]) {
  int res;
  uint8_t data[50] = {0};
  uint32_t crc;

  strncpy((char *)data, (const char *)cardPassword, 25);
  memcpy(data + 25, tempPassword, 25);

  if (isConnected) {

    Command cmd(CMD_USER_AUTHENTICATE, data, sizeof(data));
    res = sendCommand(&cmd);
    crc = cmd.crc;

    // remove the card password from memory
    memset(data, 0, sizeof(data));

    if (res == -1) {
      return -1; // communication error
    } else {     // sending the command was successful
      Sleep::msleep(1000);

      Response resp;
      resp.getResponse(this);

      if (crc == resp.lastCommandCRC) { // the response was for the last command
        if (resp.lastCommandStatus == CMD_STATUS_OK) {
          memcpy(userTemporaryPassword, tempPassword, 25);
          validUserPassword = true;
          return 0; // OK
        } else if (resp.lastCommandStatus == CMD_STATUS_WRONG_PASSWORD) {
          validUserPassword = false;
          return -3; // WRONG_PASSWORD
        }
      }
    }
  }
  return -2; // NOT CONNECTED
}

/*******************************************************************************

  authorize

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::authorize(Command *authorizedCmd) {
  /**
   * NK Pro 0.7 RTM.1 loses temporary password on cmd_*_authorize
   */
  authorizedCmd->generateCRC();
  uint32_t crc = authorizedCmd->crc;

  uint8_t data[29];

  int res;

  memcpy(data, &crc, 4);
  memcpy(data + 4, adminTemporaryPassword, 25);

  if (isConnected) {
    Command cmd(CMD_AUTHORIZE, data, sizeof(data));

    res = sendCommand(&cmd);

    if (res == -1) {

      return -1;
    } else {
      Sleep::msleep(200);
      Response resp;
      resp.getResponse(this);

      if (cmd.crc == resp.lastCommandCRC) { // the response was for the last command
        if (resp.lastCommandStatus == CMD_STATUS_OK) {

          return 0;
        }
      }

      return -2;
    }
  }
  return -1;
}

/*******************************************************************************

  userAuthorize

  Reviews
  Date      Reviewer        Info
  10.08.14  SN              First review

*******************************************************************************/

int Device::userAuthorize(Command *authorizedCmd) {
  /**
   * NK Pro 0.7 RTM.1 loses temporary password on cmd_*_authorize
   */
  authorizedCmd->generateCRC();
  uint32_t crc = authorizedCmd->crc;
  uint8_t data[29];

  memcpy(data, &crc, 4);
  memcpy(data + 4, userTemporaryPassword, 25);

  if (!isConnected) {
    return -1; // connection problem
  }

  int res;
  Command cmd(CMD_USER_AUTHORIZE, data, 29);
  res = sendCommand(&cmd);
  if (res == -1) {
    return -1; // connection problem
  }

  Sleep::msleep(200);

  Response resp;
  resp.getResponse(this);
  if (cmd.crc == resp.lastCommandCRC) { // the response was for the last command
    if (resp.lastCommandStatus == CMD_STATUS_OK) {
      return 0; // OK!
    }
  }
  return -2; // wrong CRC or not OK status
}

// returns 0 on success
int Device::unlockUserPassword(uint8_t *adminPassword) {
  int res;

  if (isConnected) {
    Command *cmd = new Command(CMD_UNLOCK_USER_PASSWORD, adminPassword, STICK20_PASSOWRD_LEN + 2);
    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else // sending the command was successful
    {
      Sleep::msleep(800);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          HID_Stick20Configuration_st.UserPwRetryCount = 3;
          Stick20_ConfigurationChanged = TRUE;
          delete cmd;

          return 0;
        }
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
    delete cmd;
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  unlockUserPasswordStick10

  Changes
  Date      Author        Info
  15.12.14  GG            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::unlockUserPasswordStick10(uint8_t *data) {
  int res;

  if (isConnected) {
    Command *cmd = new Command(CMD_UNLOCK_USER_PASSWORD, data, 50);

    res = sendCommand(cmd);

    if (res == -1) {
      return ERR_SENDING;
    } else // sending the command was successful
    {
      Sleep::msleep(1000);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          HID_Stick20Configuration_st.UserPwRetryCount = 3;
          Stick20_ConfigurationChanged = TRUE;
          return CMD_STATUS_OK;
        } else {
          return resp->lastCommandStatus;
        }
      } else {
        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  changeUserPin

  Changes
  Date      Author        Info
  20.10.14  GG            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::changeUserPin(uint8_t *old_pin, uint8_t *new_pin) {
  int res;

  uint8_t data[50];

  memcpy(data, old_pin, 25);
  memcpy(data + 25, new_pin, 25);

  if (isConnected) {
    Command *cmd = new Command(CMD_CHANGE_USER_PIN, data, 50);

    res = sendCommand(cmd);

    // remove the user password from memory
    delete cmd;

    memset(data, 0, sizeof(data));

    if (-1 == res) {
      return ERR_SENDING;
    } else {
      Sleep::msleep(800);
      Response *resp = new Response();

      resp->getResponse(this);

      // if (cmd->crc == resp->lastCommandCRC)
      {
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          return CMD_STATUS_OK;
        } else {
          return CMD_STATUS_WRONG_PASSWORD;
        }
      }
      /* else { return ERR_WRONG_RESPONSE_CRC; } */
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************
  isAesSupported

  Changes
  Date      Author        Info
  27.10.14  GG            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::isAesSupported(uint8_t *password) {
  int res;

  if (isConnected) {
    Command *cmd = new Command(CMD_DETECT_SC_AES, password, strlen((const char *)password));

    res = sendCommand(cmd);

    if (-1 == res) {
      delete cmd;

      return ERR_SENDING;
    } else // sending the command was successful
    {
      Sleep::msleep(2000);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        if (CMD_STATUS_OK == resp->lastCommandStatus) {
          // validUserPassword = true;
        }
        delete cmd;

        return resp->lastCommandStatus;
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  changeAdminPin

  Changes
  Date      Author        Info
  20.10.14  GG            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::changeAdminPin(uint8_t *old_pin, uint8_t *new_pin) {
  int res;

  uint8_t data[50];

  memcpy(data, old_pin, 25);
  memcpy(data + 25, new_pin, 25);

  if (isConnected) {
    Command *cmd = new Command(CMD_CHANGE_ADMIN_PIN, data, 50);

    res = sendCommand(cmd);

    // remove the user password from memory
    delete cmd;

    memset(data, 0, sizeof(data));

    if (-1 == res) {
      return ERR_SENDING;
    } else {
      Sleep::msleep(800);
      Response *resp = new Response();

      resp->getResponse(this);

      // if (cmd->crc == resp->lastCommandCRC)
      {
        if (resp->lastCommandStatus == CMD_STATUS_OK) {
          return CMD_STATUS_OK;
        } else {
          return CMD_STATUS_WRONG_PASSWORD;
        }
      }
      /* else { return ERR_WRONG_RESPONSE_CRC; } */
    }
  }
  return ERR_NOT_CONNECTED;
}

int Device::lockDevice(void) {
  uint8_t data[29];

  int res;

  if (isConnected) {
    Command *cmd = new Command(CMD_LOCK_DEVICE, data, 0);

    res = sendCommand(cmd);

    if (res == -1) {
      delete cmd;

      return ERR_SENDING;
    } else // sending the command was successful
    {
      Sleep::msleep(500);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        delete cmd;

        return (TRUE);
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

int Device::factoryReset(const char *password) {
  uint8_t n;

  int res;

  if (isConnected) {
    n = strlen(password);
    Command *cmd = new Command(CMD_FACTORY_RESET, (uint8_t *)password, n);

    res = sendCommand(cmd);

    if (-1 == res) {
      delete cmd;
      return ERR_SENDING;
    } else {
      Sleep::msleep(1000);
      Response *resp = new Response();
      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        delete cmd;
        return resp->lastCommandStatus;
      } else {
        delete cmd;
        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  buildAesKey

  Changes
  Date      Author        Info
  03.11.14  GG            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
int Device::buildAesKey(uint8_t *password) {
  int res;

  if (isConnected) {
    Command *cmd = new Command(CMD_NEW_AES_KEY, password, strlen((const char *)password));
    res = sendCommand(cmd);

    if (-1 == res) {
      delete cmd;

      return ERR_SENDING;
    } else // sending the command was successful
    {
      Sleep::msleep(4000);
      Response *resp = new Response();

      resp->getResponse(this);

      if (cmd->crc == resp->lastCommandCRC) {
        delete cmd;

        return resp->lastCommandStatus;
      } else {
        delete cmd;

        return ERR_WRONG_RESPONSE_CRC;
      }
    }
  }
  return ERR_NOT_CONNECTED;
}

/*******************************************************************************

    Here starts the new commands for stick 2.0

*******************************************************************************/

/*******************************************************************************

  stick20EnableCryptedPartition

  Send the command

    STICK20_CMD_ENABLE_CRYPTED_PARI

  with the password to cryptostick 2.0

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

#define CS20_MAX_PASSWORD_LEN 30

bool Device::stick20EnableCryptedPartition(uint8_t *password) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check password length
  n = strlen((const char *)password);
  if (CS20_MAX_PASSWORD_LEN <= n) {
    return (false);
  }

  cmd = new Command(STICK20_CMD_ENABLE_CRYPTED_PARI, password, n);
  res = sendCommand(cmd);

  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20DisableCryptedPartition

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20DisableCryptedPartition(void) {
  int res;

  Command *cmd;

  cmd = new Command(STICK20_CMD_DISABLE_CRYPTED_PARI, NULL, 0);
  res = sendCommand(cmd);

  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20EnableHiddenCryptedPartition

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20EnableHiddenCryptedPartition(uint8_t *password) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check password length
  n = strlen((const char *)password);

  if (CS20_MAX_PASSWORD_LEN <= n) {
    return (false);
  }

  cmd = new Command(STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI, password, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20DisableHiddenCryptedPartition

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20DisableHiddenCryptedPartition(void) {
  int res;

  Command *cmd;

  cmd = new Command(STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI, NULL, 0);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20EnableFirmwareUpdate

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20EnableFirmwareUpdate(uint8_t *password) {
  uint8_t n;
  int res;
  Command *cmd;

  // Check password length
  n = strlen((const char *)password);
  if (CS20_MAX_UPDATE_PASSWORD_LEN + 1 < n) { //+1 for [0]='p'
    return (false);
  }

  cmd = new Command(STICK20_CMD_ENABLE_FIRMWARE_UPDATE, password, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

bool Device::stick20NewUpdatePassword(uint8_t *old_password, uint8_t *new_password) {
  uint8_t n;
  int res;
  const int HID_STRING_LEN = 50;
  uint8_t SendString[HID_STRING_LEN] = {0};
  Command *cmd;

  // Check password length
  n = strlen((const char *)old_password);
  if (CS20_MAX_UPDATE_PASSWORD_LEN < n) {
    return (false);
  }

  n = strlen((const char *)new_password);
  if (CS20_MAX_UPDATE_PASSWORD_LEN < n) {
    return (false);
  }

  STRNCPY((char *)&SendString[1], sizeof(SendString) - 1, (char *)old_password, CS20_MAX_UPDATE_PASSWORD_LEN);
  STRNCPY((char *)&SendString[22], sizeof(SendString) - 22, (char *)new_password, CS20_MAX_UPDATE_PASSWORD_LEN);

  cmd = new Command(STICK20_CMD_CHANGE_UPDATE_PIN, SendString, sizeof(SendString));
  res = sendCommand(cmd);
  bool sentWithSuccess = res > 0;
  delete cmd;

  Stick20ResponseTask ResponseTask(NULL, this, NULL);
  ResponseTask.NoStopWhenStatusOK();
  ResponseTask.GetResponse();
  bool commandSuccess = ResponseTask.ResultValue == TRUE;

  /*
    Sleep::msleep(2000);
    Response *resp = new Response();
    resp->getResponse(this);
    delete resp;
  */

  return sentWithSuccess && commandSuccess;
}

/*******************************************************************************

  stick20ExportFirmware

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20ExportFirmware(uint8_t *password) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check password length
  n = strlen((const char *)password);
  if (CS20_MAX_PASSWORD_LEN <= n) {
    return (false);
  }

  cmd = new Command(STICK20_CMD_EXPORT_FIRMWARE_TO_FILE, password, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20CreateNewKeys

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20CreateNewKeys(uint8_t *password) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check password length
  n = strlen((const char *)password);
  if (CS20_MAX_PASSWORD_LEN <= n) {
    return (false);
  }

  cmd = new Command(STICK20_CMD_GENERATE_NEW_KEYS, password, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20FillSDCardWithRandomChars

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20FillSDCardWithRandomChars(uint8_t *password, uint8_t VolumeFlag) {
  uint8_t n;

  int res;

  Command *cmd;

  uint8_t data[CS20_MAX_PASSWORD_LEN + 2];

  // Check password length
  n = strlen((const char *)password);
  if (CS20_MAX_PASSWORD_LEN <= n) {
    return (false);
  }

  data[0] = VolumeFlag;

  STRCPY((char *)&data[1], sizeof(data) - 1, (char *)password);

  cmd = new Command(STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS, data, n + 1);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20SetupHiddenVolume

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20SetupHiddenVolume(void) {
  int res;

  Command *cmd;

  cmd = new Command(STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP, NULL, 0);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20GetPasswordMatrix

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20GetPasswordMatrix(void) {
  int res;

  Command *cmd;

  cmd = new Command(STICK20_CMD_SEND_PASSWORD_MATRIX, NULL, 0);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20SendPasswordMatrixPinData

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20SendPasswordMatrixPinData(uint8_t *Pindata) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check pin data length
  n = strlen((const char *)Pindata);
  if (STICK20_PASSOWRD_LEN + 2 <= n) // Kind byte + End byte 0
  {
    return (false);
  }

  cmd = new Command(STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA, Pindata, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20SendPasswordMatrixSetup

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20SendPasswordMatrixSetup(uint8_t *Setupdata) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check pin data length
  n = strlen((const char *)Setupdata);
  if (STICK20_PASSOWRD_LEN + 1 <= n) {
    return (false);
  }

  cmd = new Command(STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP, Setupdata, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20GetStatusData

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20GetStatusData() {
  int res;

  Command *cmd;

  HID_Stick20Init(); // Clear data

  cmd = new Command(STICK20_CMD_GET_DEVICE_STATUS, NULL, 0);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

bool Device::stick20SendPassword(uint8_t *Pindata) {
  uint8_t n;
  bool success;
  Command *cmd;

  // Check pin data length
  n = strlen((const char *)Pindata);
  if (STICK20_PASSOWRD_LEN + 2 <= n) // Kind byte + End byte 0
  {
    return false;
  }

  cmd = new Command(STICK20_CMD_SEND_PASSWORD, Pindata, n);
  success = sendCommand(cmd) > 0;
  delete cmd;

  return success;
}

// FIXME stick20SendNewPassword is a duplicate of stick20SendPassword
bool Device::stick20SendNewPassword(uint8_t *NewPindata) {
  uint8_t n;
  int success;
  Command *cmd;

  // Check pin data length
  n = strlen((const char *)NewPindata);
  if (STICK20_PASSOWRD_LEN + 2 <= n) // Kind byte + End byte 0
  {
    return (false);
  }

  cmd = new Command(STICK20_CMD_SEND_NEW_PASSWORD, NewPindata, n);
  success = sendCommand(cmd) > 0;
  delete cmd;

  return success;
}

/*******************************************************************************

  stick20SendSetReadonlyToUncryptedVolume

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int Device::stick20SendSetReadonlyToUncryptedVolume(uint8_t *Pindata) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check pin data length
  n = strlen((const char *)Pindata);
  if (STICK20_PASSOWRD_LEN + 2 <= n) // Kind byte + End byte 0
  {
    return (false);
  }

  cmd = new Command(STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN, Pindata, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20SendSetReadwriteToUncryptedVolume

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int Device::stick20SendSetReadwriteToUncryptedVolume(uint8_t *Pindata) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check pin data length
  n = strlen((const char *)Pindata);
  if (STICK20_PASSOWRD_LEN + 2 <= n) // Kind byte + End byte 0
  {
    return (false);
  }

  cmd = new Command(STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN, Pindata, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20SendClearNewSdCardFound

  Changes
  Date      Author          Info
  11.02.14  RB              Implementation: New SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20SendClearNewSdCardFound(uint8_t *Pindata) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check pin data length
  n = strlen((const char *)Pindata);
  if (STICK20_PASSOWRD_LEN + 2 <= n) // Kind byte + End byte 0
  {
    return (false);
  }

  cmd = new Command(STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND, Pindata, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20SendStartup

  Changes
  Date      Author          Info
  28.03.14  RB              Send startup and get startup infos

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20SendStartup(uint64_t localTime) {
  uint8_t data[30];

  int res;

  Command *cmd;

  memcpy(data, &localTime, 8);

  cmd = new Command(STICK20_CMD_SEND_STARTUP, data, 8);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20SendHiddenVolumeSetup

  Changes
  Date      Author          Info
  25.04.14  RB              Send setup of a hidden volume

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20SendHiddenVolumeSetup(HiddenVolumeSetup_tst *HV_Data_st) {
  uint8_t data[30];

  int res;

  Command *cmd;

  // uint8_t SizeCheck_data[1 - 2*(sizeof(HiddenVolumeSetup_tst) > 30)];

  memcpy(data, HV_Data_st, sizeof(HiddenVolumeSetup_tst));

  cmd = new Command(STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP, data, sizeof(HiddenVolumeSetup_tst));
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20LockFirmware

  Changes
  Date      Author        Info
  02.06.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20LockFirmware(uint8_t *password) {
  uint8_t n;

  int res;

  Command *cmd;

  // Check password length
  n = strlen((const char *)password);
  if (CS20_MAX_PASSWORD_LEN <= n) {
    return (false);
  }

  cmd = new Command(STICK20_CMD_SEND_LOCK_STICK_HARDWARE, password, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

/*******************************************************************************

  stick20ProductionTest

  Changes
  Date      Author        Info
  07.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20ProductionTest(void) {
  //  QElapsedTimer timer1;
  uint8_t n;

  int res;

  uint8_t TestData[10];

  Command *cmd;

  n = 0;

  cmd = new Command(STICK20_CMD_PRODUCTION_TEST, TestData, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;

  //  timer1.start ();

  Stick20ResponseTask ResponseTask(NULL, this, NULL);
  ResponseTask.NoStopWhenStatusOK();
  ResponseTask.GetResponse();

  //  qDebug() << " " << timer1.elapsed() << "milliseconds";
  return success;
}

/*******************************************************************************

  stick20GetDebugData

  Changes
  Date      Author        Info
  10.12.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20GetDebugData(void) {
  uint8_t n;

  int res;

  uint8_t TestData[10];

  Command *cmd;

  n = 0;

  cmd = new Command(STICK20_CMD_SEND_DEBUG_DATA, TestData, n);
  res = sendCommand(cmd);
  bool success = res > 0;
  delete cmd;
  return success;
}

bool Device::is_nkpro_07_rtm1() const { return (firmwareVersion[0] == 7 && firmwareVersion[1] == 0 && !activStick20); }
bool Device::is_nkpro_08_rtm2() const { return (firmwareVersion[0] == 8 && firmwareVersion[1] == 0 && !activStick20); }
uint8_t Device::get_major_firmware_version() const {
  if (is_nk_pro()){
    return firmwareVersion[0];
  } else if (is_nk_storage()){
    return HID_Stick20Configuration_st.VersionInfo_au8[1];
  }
  return firmwareVersion[0];
}
bool Device::is_nk_pro() const {return !activStick20;}
bool Device::is_nk_storage() const {return activStick20;}

/*******************************************************************************

  getCode

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/
/*
   int Device::getCode(uint8_t slotNo, uint64_t challenge,uint64_t lastTOTPTime,uint8_t
   lastInterval,uint8_t result[18]) {

   qDebug() << "getting code" << slotNo; int res; uint8_t data[30];

   data[0]=slotNo;

   memcpy(data+ 1,&challenge,8);

   memcpy(data+ 9,&lastTOTPTime,8); // Time of challenge: Warning: it's better to tranfer time and
   interval, to avoid attacks with wrong timestamps
   memcpy(data+17,&lastInterval,1);

   if (isConnected){ qDebug() << "sending command";

   Command *cmd=new Command(CMD_GET_CODE,data,18); res=sendCommand(cmd);

   if (res==-1) return -1; else{ //sending the command was successful //return cmd->crc; qDebug() <<
   "command sent"; Sleep::msleep(100); Response
   *resp=new Response(); resp->getResponse(this);


   if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command if
   (resp->lastCommandStatus==CMD_STATUS_OK){
   memcpy(result,resp->data,18);

   }

   }

   return 0; }

   }

   return -1;

   } */

/*******************************************************************************

  testHOTP

  Reviews
  Date      Reviewer        Info
  06.08.14  SN              First review

*******************************************************************************/
// START - OTP Test Routine --------------------------------
/*
   uint16_t Device::testHOTP(uint16_t tests_number,uint8_t counter_number){

   uint8_t data[10]; uint16_t result; int res;

   data[0] =counter_number;

   memcpy(data+1,&tests_number,2);

   if (isConnected){ qDebug() << "sending command";

   Command *cmd=new Command(CMD_TEST_COUNTER,data,3); res=sendCommand(cmd);

   if (res==-1) return -2; else{ //sending the command was successful //return cmd->crc; qDebug() <<
   "command sent"; Sleep::msleep(100*tests_number);
   Response *resp=new Response(); resp->getResponse(this);


   if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command if
   (resp->lastCommandStatus==CMD_STATUS_OK){
   //memcpy(&result,resp->data,2); result = *((uint16_t *)resp->data); return result; }

   }

   return -1; }

   }

   return -2;

   } */
// END - OTP Test Routine ----------------------------------

/*******************************************************************************

  testTOTP

  Reviews
  Date      Reviewer        Info
  06.08.14  SN              First review

*******************************************************************************/

// START - OTP Test Routine --------------------------------
/*
   uint16_t Device::testTOTP(uint16_t tests_number){

   uint8_t data[10]; uint16_t result; int res;

   data[0] = 1;

   memcpy(data+1,&tests_number,2);

   if (isConnected){ qDebug() << "sending command";

   Command *cmd=new Command(CMD_TEST_TIME,data,3); res=sendCommand(cmd);

   if (res==-1) return -2; else{ //sending the command was successful //return cmd->crc; qDebug() <<
   "command sent"; Sleep::msleep(100*tests_number);
   Response *resp=new Response(); resp->getResponse(this);


   if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command if
   (resp->lastCommandStatus==CMD_STATUS_OK){
   //memcpy(&result,resp->data,2); result = *((uint16_t *)resp->data); return result; }

   }

   return -1; }

   }

   return -2;

   } */
// END - OTP Test Routine ----------------------------------
