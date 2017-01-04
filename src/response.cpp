/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *                      Parts Rudolf Boeddeker  Date: 2013-08-13
 *
 * This file is part of Nitrokey.
 *
 * Nitrokey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>

#include "mcvs-wrapper.h"
#include "response.h"
#include "sleep.h"
#include "string.h"

#include "stick20responsedialog.h"

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

  Response

  Constructor Response

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

Response::Response() {
  deviceStatus = 0;
  lastCommandType = 0;
  lastCommandCRC = 0;
  lastCommandStatus = 0;
  responseCRC = 0;
  DebugResponseFlag = TRUE;

  HID_Stick20Status_st.CommandCounter_u8 = 0;
  HID_Stick20Status_st.LastCommand_u8 = 0;
  HID_Stick20Status_st.Status_u8 = 0;
  HID_Stick20Status_st.ProgressBarValue_u8 = 0;
}

/*******************************************************************************

  DebugResponse

  Changes
  Date      Author          Info
  10.03.14  RB              Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/
void Response::DebugResponse() {
  char text[1000];

  char text1[1000];

  int i;

  static int Counter = 0;

  if (FALSE == DebugResponseFlag) // Don't log debug data
  {
    return;
  }

  SNPRINTF(text, sizeof(text), "%6d :getResponse : ", Counter);

  Counter++;
  DebugAppendTextGui(text);
  for (i = 0; i <= 64; i++) {
    SNPRINTF(text, sizeof(text), "%02x ", (unsigned char)reportBuffer[i]);
    DebugAppendTextGui(text);
  }

  SNPRINTF(text, sizeof(text), "\n");
  DebugAppendTextGui(text);

  // Check device status = deviceStatus = reportBuffer[1]
  switch (deviceStatus) {
  case CMD_STATUS_OK:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_OK\n");
    break;
  case CMD_STATUS_WRONG_CRC:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_WRONG_CRC\n");
    break;
  case CMD_STATUS_WRONG_SLOT:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_WRONG_SLOT\n");
    break;
  case CMD_STATUS_SLOT_NOT_PROGRAMMED:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_SLOT_NOT_PROGRAMMED\n");
    break;
  case CMD_STATUS_WRONG_PASSWORD:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_WRONG_PASSWORD\n");
    break;
  case CMD_STATUS_NOT_AUTHORIZED:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_NOT_AUTHORIZED\n");
    break;
  case CMD_STATUS_TIMESTAMP_WARNING:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_TIMESTAMP_WARNING\n");
    break;
  case CMD_STATUS_NO_NAME_ERROR:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_NO_NAME_ERROR\n");
    break;
  case CMD_STATUS_NOT_SUPPORTED:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_NOT_SUPPORTED\n");
    break;
  case CMD_STATUS_UNKNOWN_COMMAND:
    DebugAppendTextGui((char *)"         Device status       : CMD_STATUS_UNKNOWN_COMMAND\n");
    break;

  default:
    DebugAppendTextGui((char *)"         Device status       : Unknown\n");
    break;
  }

  // Check last command = lastCommandType = reportBuffer[2]
  switch (lastCommandType) {
  case CMD_GET_STATUS:
    DebugAppendTextGui((char *)"         Last command        : CMD_GET_STATUS\n");
    break;
  case CMD_WRITE_TO_SLOT:
    DebugAppendTextGui((char *)"         Last command        : CMD_WRITE_TO_SLOT\n");
    break;
  case CMD_READ_SLOT_NAME:
    STRNCPY(text1, sizeof(text1), data, 15);
    text1[15] = 0;
    SNPRINTF(text, sizeof(text), "         Last command        : CMD_READ_SLOT_NAME -%s-\n", text1);
    DebugAppendTextGui(text);
    break;
  case CMD_READ_SLOT:
    DebugAppendTextGui((char *)"         Last command        : CMD_READ_SLOT\n");
    break;
  case CMD_GET_CODE:
    DebugAppendTextGui((char *)"         Last command        : CMD_GET_CODE\n");
    break;
  case CMD_WRITE_CONFIG:
    DebugAppendTextGui((char *)"         Last command        : CMD_WRITE_CONFIG\n");
    break;
  case CMD_ERASE_SLOT:
    DebugAppendTextGui((char *)"         Last command        : CMD_ERASE_SLOT\n");
    break;
  case CMD_FIRST_AUTHENTICATE:
    DebugAppendTextGui((char *)"         Last command        : CMD_FIRST_AUTHENTICATE\n");
    break;
  case CMD_AUTHORIZE:
    DebugAppendTextGui((char *)"         Last command        : CMD_AUTHORIZE\n");
    break;
  case CMD_GET_PASSWORD_RETRY_COUNT:
    DebugAppendTextGui((char *)"         Last command        : CMD_GET_PASSWORD_RETRY_COUNT\n");
    break;
  case CMD_CLEAR_WARNING:
    DebugAppendTextGui((char *)"         Last command        : CMD_CLEAR_WARNING\n");
    break;

  case CMD_GET_PW_SAFE_SLOT_STATUS:
    DebugAppendTextGui((char *)"         Last command        : CMD_GET_PW_SAFE_SLOT_STATUS\n");
    break;
  case CMD_GET_PW_SAFE_SLOT_NAME:
    DebugAppendTextGui((char *)"         Last command        : CMD_GET_PW_SAFE_SLOT_NAME\n");
    break;
  case CMD_GET_PW_SAFE_SLOT_PASSWORD:
    DebugAppendTextGui((char *)"         Last command        : CMD_GET_PW_SAFE_SLOT_PASSWORD\n");
    break;

  case CMD_GET_PW_SAFE_SLOT_LOGINNAME:
    DebugAppendTextGui((char *)"         Last command        : CMD_GET_PW_SAFE_SLOT_LOGINNAME\n");
    break;
  case CMD_SET_PW_SAFE_SLOT_DATA_1:
    DebugAppendTextGui((char *)"         Last command        : CMD_SET_PW_SAFE_SLOT_DATA_1\n");
    break;
  case CMD_SET_PW_SAFE_SLOT_DATA_2:
    DebugAppendTextGui((char *)"         Last command        : CMD_SET_PW_SAFE_SLOT_DATA_2\n");
    break;
  case CMD_PW_SAFE_ERASE_SLOT:
    DebugAppendTextGui((char *)"         Last command        : CMD_PW_SAFE_ERASE_SLOT\n");
    break;
  case CMD_PW_SAFE_ENABLE:
    DebugAppendTextGui((char *)"         Last command        : CMD_PW_SAFE_ENABLE\n");
    break;
  case CMD_PW_SAFE_INIT_KEY:
    DebugAppendTextGui((char *)"         Last command        : CMD_PW_SAFE_INIT_KEY\n");
    break;
  case CMD_PW_SAFE_SEND_DATA:
    DebugAppendTextGui((char *)"         Last command        : CMD_PW_SAFE_SEND_DATA");
    break;
  case CMD_SD_CARD_HIGH_WATERMARK:
    DebugAppendTextGui((char *)"         Last command        : CMD_SD_CARD_HIGH_WATERMARK");
    break;

  case CMD_SET_TIME:
    DebugAppendTextGui((char *)"         Last command        : CMD_SET_TIME\n");
    break;
  case CMD_TEST_COUNTER:
    DebugAppendTextGui((char *)"         Last command        : CMD_TEST_COUNTER\n");
    break;
  case CMD_TEST_TIME:
    DebugAppendTextGui((char *)"         Last command        : CMD_TEST_TIME\n");
    break;
  case CMD_USER_AUTHENTICATE:
    DebugAppendTextGui((char *)"         Last command        : CMD_USER_AUTHENTICATE\n");
    break;
  case CMD_GET_USER_PASSWORD_RETRY_COUNT:
    DebugAppendTextGui(
        (char *)"         Last command        : CMD_GET_USER_PASSWORD_RETRY_COUNT\n");
    break;
  case CMD_USER_AUTHORIZE:
    DebugAppendTextGui((char *)"         Last command        : CMD_USER_AUTHORIZE\n");
    break;
  case CMD_UNLOCK_USER_PASSWORD:
    DebugAppendTextGui((char *)"         Last command        : CMD_UNLOCK_USER_PASSWORD\n");
    break;
  case CMD_LOCK_DEVICE:
    DebugAppendTextGui((char *)"         Last command        : CMD_LOCK_DEVICE");
    break;
  case CMD_DETECT_SC_AES:
    DebugAppendTextGui((char *)"         Last command        : CMD_DETECT_SC_AES\n");
    break;
  case CMD_NEW_AES_KEY:
    DebugAppendTextGui((char *)"         Last command        : CMD_NEW_AES_KEY:\n");
    break;
    case CMD_SEND_OTP_DATA:
    DebugAppendTextGui((char *)"         Last command        : CMD_SEND_OTP_DATA:\n");
    break;

  case STICK20_CMD_ENABLE_CRYPTED_PARI:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_ENABLE_CRYPTED_PARI           \n");
    break;
  case STICK20_CMD_DISABLE_CRYPTED_PARI:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_DISABLE_CRYPTED_PARI          \n");
    break;
  case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI    \n");
    break;
  case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI   \n");
    break;
  case STICK20_CMD_ENABLE_FIRMWARE_UPDATE:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_ENABLE_FIRMWARE_UPDATE        \n");
    break;
  case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_EXPORT_FIRMWARE_TO_FILE       \n");
    break;
  case STICK20_CMD_GENERATE_NEW_KEYS:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_GENERATE_NEW_KEYS             \n");
    break;
  case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS\n");
    break;
  case STICK20_CMD_WRITE_STATUS_DATA:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_WRITE_STATUS_DATA             \n");
    break;
  case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN \n");
    break;
  case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN\n");
    break;
  case STICK20_CMD_SEND_PASSWORD_MATRIX:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_SEND_PASSWORD_MATRIX          \n");
    break;
  case STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA  \n");
    break;
  case STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP    \n");
    break;
  case STICK20_CMD_GET_DEVICE_STATUS:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_GET_DEVICE_STATUS             \n");
    break;
  case STICK20_CMD_SEND_DEVICE_STATUS:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_SEND_DEVICE_STATUS            \n");
    break;

  case STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD\n");
    break;
  case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP\n");
    break;
  case STICK20_CMD_SEND_PASSWORD:
    DebugAppendTextGui((char *)"         Last command        : STICK20_CMD_SEND_PASSWORD\n");
    break;
  case STICK20_CMD_SEND_NEW_PASSWORD:
    DebugAppendTextGui((char *)"         Last command        : STICK20_CMD_SEND_NEW_PASSWORD\n");
    break;
  case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND\n");
    break;
  case STICK20_CMD_SEND_STARTUP:
    DebugAppendTextGui((char *)"         Last command        : STICK20_CMD_SEND_STARTUP\n");
    break;
  case STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED\n");
    break;
  case STICK20_CMD_SEND_LOCK_STICK_HARDWARE:
    DebugAppendTextGui(
        (char *)"         Last command        : STICK20_CMD_SEND_LOCK_STICK_HARDWARE\n");
    break;
  case STICK20_CMD_PRODUCTION_TEST:
    DebugAppendTextGui((char *)"         Last command        : STICK20_CMD_PRODUCTION_TEST\n");
    break;
  default:
    DebugAppendTextGui((char *)"         Last command        : Unknown\n");
    break;
  }

  // Check last command status = lastCommandStatus = reportBuffer[7]
  switch (lastCommandStatus) {
  case CMD_STATUS_OK:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_OK\n");
    break;
  case CMD_STATUS_WRONG_CRC:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_WRONG_CRC\n");
    break;
  case CMD_STATUS_WRONG_SLOT:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_WRONG_SLOT\n");
    break;
  case CMD_STATUS_SLOT_NOT_PROGRAMMED:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_SLOT_NOT_PROGRAMMED\n");
    break;
  case CMD_STATUS_WRONG_PASSWORD:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_WRONG_PASSWORD\n");
    break;
  case CMD_STATUS_NOT_AUTHORIZED:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_NOT_AUTHORIZED\n");
    break;
  case CMD_STATUS_TIMESTAMP_WARNING:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_TIMESTAMP_WARNING\n");
    break;
  case CMD_STATUS_NO_NAME_ERROR:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_NO_NAME_ERROR\n");
    break;
  case CMD_STATUS_NOT_SUPPORTED:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_NOT_SUPPORTED\n");
    break;
  case CMD_STATUS_UNKNOWN_COMMAND:
    DebugAppendTextGui((char *)"         Last command status : CMD_STATUS_UNKNOWN_COMMAND\n");
    break;
  default:
    DebugAppendTextGui((char *)"         Last command status : Unknown\n");
    break;
  }

  // lastCommandCRC = ((uint32_t *)(reportBuffer+3))[0];
  // responseCRC = ((uint32_t *)(reportBuffer+61))[0];
}

/*******************************************************************************

  getResponse

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/
int Response::getResponse(Device *device) {
  int res;

  memset(reportBuffer, 0, sizeof(reportBuffer));

  if (NULL == device->dev_hid_handle) {
    return -1;
  }

  res = hid_get_feature_report(device->dev_hid_handle, reportBuffer, sizeof(reportBuffer));
  if (res == -1) {
    return -1;
  }
  deviceStatus = reportBuffer[1];
  lastCommandType = reportBuffer[2];
  lastCommandCRC = reinterpret_cast<uint32_t *>(reportBuffer + 3)[0];
  lastCommandStatus = reportBuffer[7];
  responseCRC = reinterpret_cast<uint32_t *>(reportBuffer + 61)[0];

  size_t len = std::min(sizeof(data), sizeof(reportBuffer) - 8);
  memcpy(data, reportBuffer + 8, len);

  // Copy Stick 2.0 status vom HID response data
  memcpy((void *)&HID_Stick20Status_st, reportBuffer + 1 + OUTPUT_CMD_RESULT_STICK20_STATUS_START,
         sizeof(HID_Stick20Status_st));

  DebugResponse();

  return 0;
}
