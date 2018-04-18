/*
 * Copyright (c) 2015-2018 Nitrokey UG
 *
 * This file is part of libnitrokey.
 *
 * libnitrokey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libnitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libnitrokey. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

#ifndef COMMAND_ID_H
#define COMMAND_ID_H
#include <stdint.h>

namespace nitrokey {
namespace proto {
    namespace stick20 {
      enum class device_status : uint8_t {
        idle = 0,
        ok,
        busy,
        wrong_password,
        busy_progressbar,
        password_matrix_ready,
        no_user_password_unlock, // FIXME: translate on receive to command status error (fix in firmware?)
        smartcard_error,
        security_bit_active
      };
      const int CMD_START_VALUE = 0x20;
      const int CMD_END_VALUE = 0x60;
    }
    namespace stick10 {
      enum class command_status : uint8_t {
          ok = 0,
          wrong_CRC,
          wrong_slot,
          slot_not_programmed,
          wrong_password  = 4,
          not_authorized,
          timestamp_warning,
          no_name_error,
          not_supported,
          unknown_command,
          AES_dec_failed
      };
      enum class device_status : uint8_t {
        ok = 0,
        busy = 1,
        error,
        received_report,
      };
    }


enum class CommandID : uint8_t {
  GET_STATUS = 0x00,
  WRITE_TO_SLOT = 0x01,
  READ_SLOT_NAME = 0x02,
  READ_SLOT = 0x03,
  GET_CODE = 0x04,
  WRITE_CONFIG = 0x05,
  ERASE_SLOT = 0x06,
  FIRST_AUTHENTICATE = 0x07,
  AUTHORIZE = 0x08,
  GET_PASSWORD_RETRY_COUNT = 0x09,
  CLEAR_WARNING = 0x0A,
  SET_TIME = 0x0B,
  TEST_COUNTER = 0x0C,
  TEST_TIME = 0x0D,
  USER_AUTHENTICATE = 0x0E,
  GET_USER_PASSWORD_RETRY_COUNT = 0x0F,
  USER_AUTHORIZE = 0x10,
  UNLOCK_USER_PASSWORD = 0x11,
  LOCK_DEVICE = 0x12,
  FACTORY_RESET = 0x13,
  CHANGE_USER_PIN = 0x14,
  CHANGE_ADMIN_PIN = 0x15,
  WRITE_TO_SLOT_2 = 0x16,
  SEND_OTP_DATA = 0x17,

  ENABLE_CRYPTED_PARI = 0x20,
  DISABLE_CRYPTED_PARI = 0x20 + 1,
  ENABLE_HIDDEN_CRYPTED_PARI = 0x20 + 2,
  DISABLE_HIDDEN_CRYPTED_PARI = 0x20 + 3,
  ENABLE_FIRMWARE_UPDATE = 0x20 + 4, //enables update mode
  EXPORT_FIRMWARE_TO_FILE = 0x20 + 5,
  GENERATE_NEW_KEYS = 0x20 + 6,
  FILL_SD_CARD_WITH_RANDOM_CHARS = 0x20 + 7,

  WRITE_STATUS_DATA = 0x20 + 8, //@unused
  ENABLE_READONLY_UNCRYPTED_LUN = 0x20 + 9,
  ENABLE_READWRITE_UNCRYPTED_LUN = 0x20 + 10,

  SEND_PASSWORD_MATRIX = 0x20 + 11, //@unused
  SEND_PASSWORD_MATRIX_PINDATA = 0x20 + 12, //@unused
  SEND_PASSWORD_MATRIX_SETUP = 0x20 + 13, //@unused

  GET_DEVICE_STATUS = 0x20 + 14,
  SEND_DEVICE_STATUS = 0x20 + 15,

  SEND_HIDDEN_VOLUME_PASSWORD = 0x20 + 16, //@unused
  SEND_HIDDEN_VOLUME_SETUP = 0x20 + 17,
  SEND_PASSWORD = 0x20 + 18,
  SEND_NEW_PASSWORD = 0x20 + 19,
  CLEAR_NEW_SD_CARD_FOUND = 0x20 + 20,

  SEND_STARTUP = 0x20 + 21,
  SEND_CLEAR_STICK_KEYS_NOT_INITIATED = 0x20 + 22,
  SEND_LOCK_STICK_HARDWARE = 0x20 + 23, //locks firmware upgrade

  PRODUCTION_TEST = 0x20 + 24,
  SEND_DEBUG_DATA = 0x20 + 25, //@unused

  CHANGE_UPDATE_PIN = 0x20 + 26,

  //added in v0.48.5
  ENABLE_ADMIN_READONLY_UNCRYPTED_LUN = 0x20 + 28,
  ENABLE_ADMIN_READWRITE_UNCRYPTED_LUN = 0x20 + 29,
  ENABLE_ADMIN_READONLY_ENCRYPTED_LUN = 0x20 + 30,
  ENABLE_ADMIN_READWRITE_ENCRYPTED_LUN = 0x20 + 31,
  CHECK_SMARTCARD_USAGE = 0x20 + 32,

  GET_PW_SAFE_SLOT_STATUS = 0x60,
  GET_PW_SAFE_SLOT_NAME = 0x61,
  GET_PW_SAFE_SLOT_PASSWORD = 0x62,
  GET_PW_SAFE_SLOT_LOGINNAME = 0x63,
  SET_PW_SAFE_SLOT_DATA_1 = 0x64,
  SET_PW_SAFE_SLOT_DATA_2 = 0x65,
  PW_SAFE_ERASE_SLOT = 0x66,
  PW_SAFE_ENABLE = 0x67,
  PW_SAFE_INIT_KEY = 0x68, //@unused
  PW_SAFE_SEND_DATA = 0x69, //@unused
  SD_CARD_HIGH_WATERMARK = 0x70,
  DETECT_SC_AES = 0x6a,
  NEW_AES_KEY = 0x6b
};

const char *commandid_to_string(CommandID id);
}
}
#endif
