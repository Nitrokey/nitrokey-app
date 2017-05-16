#include <assert.h>
#include "command_id.h"

namespace nitrokey {
namespace proto {

const char *commandid_to_string(CommandID id) {
  switch (id) {
    case CommandID::GET_STATUS:
      return "GET_STATUS";
    case CommandID::WRITE_TO_SLOT:
      return "WRITE_TO_SLOT";
    case CommandID::READ_SLOT_NAME:
      return "READ_SLOT_NAME";
    case CommandID::READ_SLOT:
      return "READ_SLOT";
    case CommandID::GET_CODE:
      return "GET_CODE";
    case CommandID::WRITE_CONFIG:
      return "WRITE_CONFIG";
    case CommandID::ERASE_SLOT:
      return "ERASE_SLOT";
    case CommandID::FIRST_AUTHENTICATE:
      return "FIRST_AUTHENTICATE";
    case CommandID::AUTHORIZE:
      return "AUTHORIZE";
    case CommandID::GET_PASSWORD_RETRY_COUNT:
      return "GET_PASSWORD_RETRY_COUNT";
    case CommandID::CLEAR_WARNING:
      return "CLEAR_WARNING";
    case CommandID::SET_TIME:
      return "SET_TIME";
    case CommandID::TEST_COUNTER:
      return "TEST_COUNTER";
    case CommandID::TEST_TIME:
      return "TEST_TIME";
    case CommandID::USER_AUTHENTICATE:
      return "USER_AUTHENTICATE";
    case CommandID::GET_USER_PASSWORD_RETRY_COUNT:
      return "GET_USER_PASSWORD_RETRY_COUNT";
    case CommandID::USER_AUTHORIZE:
      return "USER_AUTHORIZE";
    case CommandID::UNLOCK_USER_PASSWORD:
      return "UNLOCK_USER_PASSWORD";
    case CommandID::LOCK_DEVICE:
      return "LOCK_DEVICE";
    case CommandID::FACTORY_RESET:
      return "FACTORY_RESET";
    case CommandID::CHANGE_USER_PIN:
      return "CHANGE_USER_PIN";
    case CommandID::CHANGE_ADMIN_PIN:
      return "CHANGE_ADMIN_PIN";

    case CommandID::ENABLE_CRYPTED_PARI:
      return "ENABLE_CRYPTED_PARI";
    case CommandID::DISABLE_CRYPTED_PARI:
      return "DISABLE_CRYPTED_PARI";
    case CommandID::ENABLE_HIDDEN_CRYPTED_PARI:
      return "ENABLE_HIDDEN_CRYPTED_PARI";
    case CommandID::DISABLE_HIDDEN_CRYPTED_PARI:
      return "DISABLE_HIDDEN_CRYPTED_PARI";
    case CommandID::ENABLE_FIRMWARE_UPDATE:
      return "ENABLE_FIRMWARE_UPDATE";
    case CommandID::EXPORT_FIRMWARE_TO_FILE:
      return "EXPORT_FIRMWARE_TO_FILE";
    case CommandID::GENERATE_NEW_KEYS:
      return "GENERATE_NEW_KEYS";
    case CommandID::FILL_SD_CARD_WITH_RANDOM_CHARS:
      return "FILL_SD_CARD_WITH_RANDOM_CHARS";

    case CommandID::WRITE_STATUS_DATA:
      return "WRITE_STATUS_DATA";
    case CommandID::ENABLE_READONLY_UNCRYPTED_LUN:
      return "ENABLE_READONLY_UNCRYPTED_LUN";
    case CommandID::ENABLE_READWRITE_UNCRYPTED_LUN:
      return "ENABLE_READWRITE_UNCRYPTED_LUN";

    case CommandID::SEND_PASSWORD_MATRIX:
      return "SEND_PASSWORD_MATRIX";
    case CommandID::SEND_PASSWORD_MATRIX_PINDATA:
      return "SEND_PASSWORD_MATRIX_PINDATA";
    case CommandID::SEND_PASSWORD_MATRIX_SETUP:
      return "SEND_PASSWORD_MATRIX_SETUP";

    case CommandID::GET_DEVICE_STATUS:
      return "GET_DEVICE_STATUS";
    case CommandID::SEND_DEVICE_STATUS:
      return "SEND_DEVICE_STATUS";

    case CommandID::SEND_HIDDEN_VOLUME_PASSWORD:
      return "SEND_HIDDEN_VOLUME_PASSWORD";
    case CommandID::SEND_HIDDEN_VOLUME_SETUP:
      return "SEND_HIDDEN_VOLUME_SETUP";
    case CommandID::SEND_PASSWORD:
      return "SEND_PASSWORD";
    case CommandID::SEND_NEW_PASSWORD:
      return "SEND_NEW_PASSWORD";
    case CommandID::CLEAR_NEW_SD_CARD_FOUND:
      return "CLEAR_NEW_SD_CARD_FOUND";

    case CommandID::SEND_STARTUP:
      return "SEND_STARTUP";
    case CommandID::SEND_CLEAR_STICK_KEYS_NOT_INITIATED:
      return "SEND_CLEAR_STICK_KEYS_NOT_INITIATED";
    case CommandID::SEND_LOCK_STICK_HARDWARE:
      return "SEND_LOCK_STICK_HARDWARE";

    case CommandID::PRODUCTION_TEST:
      return "PRODUCTION_TEST";
    case CommandID::SEND_DEBUG_DATA:
      return "SEND_DEBUG_DATA";

    case CommandID::CHANGE_UPDATE_PIN:
      return "CHANGE_UPDATE_PIN";

    case CommandID::GET_PW_SAFE_SLOT_STATUS:
      return "GET_PW_SAFE_SLOT_STATUS";
    case CommandID::GET_PW_SAFE_SLOT_NAME:
      return "GET_PW_SAFE_SLOT_NAME";
    case CommandID::GET_PW_SAFE_SLOT_PASSWORD:
      return "GET_PW_SAFE_SLOT_PASSWORD";
    case CommandID::GET_PW_SAFE_SLOT_LOGINNAME:
      return "GET_PW_SAFE_SLOT_LOGINNAME";
    case CommandID::SET_PW_SAFE_SLOT_DATA_1:
      return "SET_PW_SAFE_SLOT_DATA_1";
    case CommandID::SET_PW_SAFE_SLOT_DATA_2:
      return "SET_PW_SAFE_SLOT_DATA_2";
    case CommandID::PW_SAFE_ERASE_SLOT:
      return "PW_SAFE_ERASE_SLOT";
    case CommandID::PW_SAFE_ENABLE:
      return "PW_SAFE_ENABLE";
    case CommandID::PW_SAFE_INIT_KEY:
      return "PW_SAFE_INIT_KEY";
    case CommandID::PW_SAFE_SEND_DATA:
      return "PW_SAFE_SEND_DATA";
    case CommandID::SD_CARD_HIGH_WATERMARK:
      return "SD_CARD_HIGH_WATERMARK";
    case CommandID::DETECT_SC_AES:
      return "DETECT_SC_AES";
    case CommandID::NEW_AES_KEY:
      return "NEW_AES_KEY";
    case CommandID::WRITE_TO_SLOT_2:
      return "WRITE_TO_SLOT_2";
      break;
    case CommandID::SEND_OTP_DATA:
      return "SEND_OTP_DATA";
      break;
  }
  return "UNKNOWN";
}
}
}
