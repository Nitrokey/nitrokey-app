#ifndef STICK10_COMMANDS_H
#define STICK10_COMMANDS_H

#include <bitset>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdint.h>
#include "device_proto.h"
#include "command.h"

#pragma pack (push,1)

namespace nitrokey {
namespace proto {



/*
 *	Stick10 protocol definition
 */
namespace stick10 {
class GetSlotName : public Command<CommandID::READ_SLOT_NAME> {
 public:
  // reachable as a typedef in Transaction
  struct CommandPayload {
    uint8_t slot_number;

    bool isValid() const { return slot_number<0x10+3; }
      std::string dissect() const {
          std::stringstream ss;
          ss << "slot_number:\t" << (int)(slot_number) << std::endl;
          return ss.str();
      }
  } __packed;

  struct ResponsePayload {
    uint8_t slot_name[15];

    bool isValid() const { return true; }
      std::string dissect() const {
          std::stringstream ss;
          ss << "slot_name:\t" << slot_name << std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload,
                      struct ResponsePayload> CommandTransaction;
};

class EraseSlot : Command<CommandID::ERASE_SLOT> {
 public:
  struct CommandPayload {
    uint8_t slot_number;

    bool isValid() const { return !(slot_number & 0xF0); }
      std::string dissect() const {
          std::stringstream ss;
          ss << "slot_number:\t" << (int)(slot_number) << std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class SetTime : Command<CommandID::SET_TIME> {
 public:
  struct CommandPayload {
    uint8_t reset;  // 0 - get time, 1 - set time
    uint64_t time;  // posix time

    bool isValid() const { return reset && reset != 1; }
      std::string dissect() const {
          std::stringstream ss;
          ss << "reset:\t" << (int)(reset) << std::endl;
          ss << "time:\t" << (time) << std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};


class WriteToHOTPSlot : Command<CommandID::WRITE_TO_SLOT> {
 public:
  struct CommandPayload {
    uint8_t slot_number;
    uint8_t slot_name[15];
    uint8_t slot_secret[20];
    union{
      uint8_t _slot_config;
        struct{
            bool use_8_digits   : 1;
            bool use_enter      : 1;
            bool use_tokenID    : 1;
        };
    };
      union{
        uint8_t slot_token_id[13]; /** OATH Token Identifier */
          struct{ /** @see https://openauthentication.org/token-specs/ */
              uint8_t omp[2];
              uint8_t tt[2];
              uint8_t mui[8];
              uint8_t keyboard_layout; //disabled feature in nitroapp as of 20160805
          } slot_token_fields;
      };
      union{
        uint64_t slot_counter;
        uint8_t slot_counter_s[8];
      } __packed;

    bool isValid() const { return !(slot_number & 0xF0); }
    std::string dissect() const {
        std::stringstream ss;
        ss << "slot_number:\t" << (int)(slot_number) << std::endl;
        ss << "slot_name:\t" << slot_name << std::endl;
        ss << "slot_secret:" << std::endl
           << ::nitrokey::misc::hexdump((const char *)(&slot_secret), sizeof slot_secret);
        ss << "slot_config:\t" << std::bitset<8>((int)_slot_config) << std::endl;
        ss << "\tuse_8_digits(0):\t" << use_8_digits << std::endl;
        ss << "\tuse_enter(1):\t" << use_enter << std::endl;
        ss << "\tuse_tokenID(2):\t" << use_tokenID << std::endl;

        ss << "slot_token_id:\t";
        for (auto i : slot_token_id)
            ss << std::hex << std::setw(2) << std::setfill('0')<< (int) i << " " ;
        ss << std::endl;
        ss << "slot_counter:\t[" << (int)slot_counter << "]\t"
         << ::nitrokey::misc::hexdump((const char *)(&slot_counter), sizeof slot_counter, false);

      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class WriteToTOTPSlot : Command<CommandID::WRITE_TO_SLOT> {
 public:
  struct CommandPayload {
    uint8_t slot_number;
    uint8_t slot_name[15];
    uint8_t slot_secret[20];
      union{
          uint8_t _slot_config;
          struct{
              bool use_8_digits   : 1;
              bool use_enter      : 1;
              bool use_tokenID    : 1;
          };
      };
      union{
          uint8_t slot_token_id[13]; /** OATH Token Identifier */
          struct{ /** @see https://openauthentication.org/token-specs/ */
              uint8_t omp[2];
              uint8_t tt[2];
              uint8_t mui[8];
              uint8_t keyboard_layout; //disabled feature in nitroapp as of 20160805
          } slot_token_fields;
      };
    uint16_t slot_interval;

    bool isValid() const { return !(slot_number & 0xF0); } //TODO check
      std::string dissect() const {
          std::stringstream ss;
          ss << "slot_number:\t" << (int)(slot_number) << std::endl;
          ss << "slot_name:\t" << slot_name << std::endl;
          ss << "slot_secret:\t" << slot_secret << std::endl;
          ss << "slot_config:\t" << std::bitset<8>((int)_slot_config) << std::endl;
          ss << "slot_token_id:\t";
          for (auto i : slot_token_id)
              ss << std::hex << std::setw(2) << std::setfill('0')<< (int) i << " " ;
          ss << std::endl;
          ss << "slot_interval:\t" << (int)slot_interval << std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class GetTOTP : Command<CommandID::GET_CODE> {
 public:
  struct CommandPayload {
    uint8_t slot_number;
    uint64_t challenge;
    uint64_t last_totp_time;
    uint8_t last_interval;

    bool isValid() const { return !(slot_number & 0xF0); }
    std::string dissect() const {
      std::stringstream ss;
      ss << "slot_number:\t" << (int)(slot_number) << std::endl;
      ss << "challenge:\t" << (challenge) << std::endl;
      ss << "last_totp_time:\t" << (last_totp_time) << std::endl;
      ss << "last_interval:\t" << (int)(last_interval) << std::endl;
      return ss.str();
    }
  } __packed;

  struct ResponsePayload {
      union {
          uint8_t whole_response[18]; //14 bytes reserved for config, but used only 1
          struct {
              uint32_t code;
              union{
                  uint8_t _slot_config;
                  struct{
                      bool use_8_digits   : 1;
                      bool use_enter      : 1;
                      bool use_tokenID    : 1;
                  };
              };
          } __packed ;
      } __packed ;

    bool isValid() const { return true; }
    std::string dissect() const {
      std::stringstream ss;
      ss << "code:\t" << (code) << std::endl;
        ss << "slot_config:\t" << std::bitset<8>((int)_slot_config) << std::endl;
        ss << "\tuse_8_digits(0):\t" << use_8_digits << std::endl;
        ss << "\tuse_enter(1):\t" << use_enter << std::endl;
        ss << "\tuse_tokenID(2):\t" << use_tokenID << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct ResponsePayload>
      CommandTransaction;
};

class GetHOTP : Command<CommandID::GET_CODE> {
 public:
  struct CommandPayload {
    uint8_t slot_number;

    bool isValid() const { return (slot_number & 0xF0); }
    std::string dissect() const {
      std::stringstream ss;
      ss << "slot_number:\t" << (int)(slot_number) << std::endl;
      return ss.str();
    }
  } __packed;

  struct ResponsePayload {
      union {
          uint8_t whole_response[18]; //14 bytes reserved for config, but used only 1
          struct {
              uint32_t code;
              union{
                  uint8_t _slot_config;
                  struct{
                      bool use_8_digits   : 1;
                      bool use_enter      : 1;
                      bool use_tokenID    : 1;
                  };
              };
          } __packed;
      } __packed;

    bool isValid() const { return true; }
    std::string dissect() const {
      std::stringstream ss;
      ss << "code:\t" << (code) << std::endl;
        ss << "slot_config:\t" << std::bitset<8>((int)_slot_config) << std::endl;
        ss << "\tuse_8_digits(0):\t" << use_8_digits << std::endl;
        ss << "\tuse_enter(1):\t" << use_enter << std::endl;
        ss << "\tuse_tokenID(2):\t" << use_tokenID << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct ResponsePayload>
      CommandTransaction;
};

class ReadSlot : Command<CommandID::READ_SLOT> {
 public:
  struct CommandPayload {
    uint8_t slot_number;

    bool isValid() const { return !(slot_number & 0xF0); }

    std::string dissect() const {
      std::stringstream ss;
      ss << "slot_number:\t" << (int)(slot_number) << std::endl;
      return ss.str();
    }
  } __packed;

  struct ResponsePayload {
    uint8_t slot_name[15];
    union{
      uint8_t _slot_config;
      struct{
        bool use_8_digits   : 1;
        bool use_enter      : 1;
        bool use_tokenID    : 1;
      };
    };
    union{
      uint8_t slot_token_id[13]; /** OATH Token Identifier */
      struct{ /** @see https://openauthentication.org/token-specs/ */
        uint8_t omp[2];
        uint8_t tt[2];
        uint8_t mui[8];
        uint8_t keyboard_layout; //disabled feature in nitroapp as of 20160805
      } slot_token_fields;
    };
    union{
      uint64_t slot_counter;
      uint8_t slot_counter_s[8];
    } __packed;

    bool isValid() const { return true; }

    std::string dissect() const {
      std::stringstream ss;
      ss << "slot_name:\t" << slot_name << std::endl;
      ss << "slot_config:\t" << std::bitset<8>((int)_slot_config) << std::endl;
      ss << "\tuse_8_digits(0):\t" << use_8_digits << std::endl;
      ss << "\tuse_enter(1):\t" << use_enter << std::endl;
      ss << "\tuse_tokenID(2):\t" << use_tokenID << std::endl;

      ss << "slot_token_id:\t";
      for (auto i : slot_token_id)
        ss << std::hex << std::setw(2) << std::setfill('0')<< (int) i << " " ;
      ss << std::endl;
      ss << "slot_counter:\t[" << (int)slot_counter << "]\t"
         << ::nitrokey::misc::hexdump((const char *)(&slot_counter), sizeof slot_counter, false);
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload,
                      struct ResponsePayload> CommandTransaction;
};

class GetStatus : Command<CommandID::GET_STATUS> {
 public:
  struct ResponsePayload {
    uint16_t firmware_version;
    union{
      uint8_t card_serial[4];
      uint32_t card_serial_u32;
    } __packed;
      union {
          uint8_t general_config[5];
          struct{
              uint8_t numlock;     /** 0-1: HOTP slot number from which the code will be get on double press, other value - function disabled */
              uint8_t capslock;    /** same as numlock */
              uint8_t scrolllock;  /** same as numlock */
              uint8_t enable_user_password;
              uint8_t delete_user_password;
          } __packed;
      } __packed;
    bool isValid() const { return enable_user_password!=delete_user_password; }

    std::string get_card_serial_hex() const {
      return nitrokey::misc::toHex(card_serial_u32);
    }

    std::string dissect() const {
      std::stringstream ss;
      ss  << "firmware_version:\t"
          << "[" << firmware_version << "]" << "\t"
          << ::nitrokey::misc::hexdump(
          (const char *)(&firmware_version), sizeof firmware_version, false);
      ss << "card_serial_u32:\t" << std::hex << card_serial_u32 << std::endl;
      ss << "card_serial:\t"
         << ::nitrokey::misc::hexdump((const char *)(card_serial),
                                      sizeof card_serial, false);
      ss << "general_config:\t"
         << ::nitrokey::misc::hexdump((const char *)(general_config),
                                      sizeof general_config, false);
        ss << "numlock:\t" << (int)numlock << std::endl;
        ss << "capslock:\t" << (int)capslock << std::endl;
        ss << "scrolllock:\t" << (int)scrolllock << std::endl;
        ss << "enable_user_password:\t" << (bool) enable_user_password << std::endl;
        ss << "delete_user_password:\t" << (bool) delete_user_password << std::endl;

        return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct EmptyPayload, struct ResponsePayload>
      CommandTransaction;
};

class GetPasswordRetryCount : Command<CommandID::GET_PASSWORD_RETRY_COUNT> {
 public:
  struct ResponsePayload {
    uint8_t password_retry_count;

    bool isValid() const { return true; }
    std::string dissect() const {
      std::stringstream ss;
      ss << " password_retry_count\t" << (int)password_retry_count << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct EmptyPayload, struct ResponsePayload>
      CommandTransaction;
};

class GetUserPasswordRetryCount
    : Command<CommandID::GET_USER_PASSWORD_RETRY_COUNT> {
 public:
  struct ResponsePayload {
    uint8_t password_retry_count;

    bool isValid() const { return true; }
    std::string dissect() const {
      std::stringstream ss;
      ss << " password_retry_count\t" << (int)password_retry_count << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct EmptyPayload, struct ResponsePayload>
      CommandTransaction;
};

    template <typename T, typename Q, int N>
    void write_array(T &ss, Q (&arr)[N]){
        for (int i=0; i<N; i++){
            ss << std::hex << std::setfill('0') << std::setw(2) << (int)arr[i] << " ";
        }
        ss << std::endl;
    };


class GetPasswordSafeSlotStatus : Command<CommandID::GET_PW_SAFE_SLOT_STATUS> {
 public:
  struct ResponsePayload {
    uint8_t password_safe_status[PWS_SLOT_COUNT];

    bool isValid() const { return true; }
      std::string dissect() const {
          std::stringstream ss;
          ss << "password_safe_status\t";
          write_array(ss, password_safe_status);
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct EmptyPayload, struct ResponsePayload>
      CommandTransaction;
};

class GetPasswordSafeSlotName : Command<CommandID::GET_PW_SAFE_SLOT_NAME> {
 public:
  struct CommandPayload {
    uint8_t slot_number;

    bool isValid() const { return !(slot_number & 0xF0); }
    std::string dissect() const {
      std::stringstream ss;
      ss << "slot_number\t" << (int)slot_number << std::endl;
      return ss.str();
    }
  } __packed;

  struct ResponsePayload {
    uint8_t slot_name[PWS_SLOTNAME_LENGTH];

    bool isValid() const { return true; }
    std::string dissect() const {
      std::stringstream ss;
      ss << " slot_name\t" << (const char*) slot_name << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload,
                      struct ResponsePayload> CommandTransaction;
};

class GetPasswordSafeSlotPassword
    : Command<CommandID::GET_PW_SAFE_SLOT_PASSWORD> {
 public:
  struct CommandPayload {
    uint8_t slot_number;

    bool isValid() const { return !(slot_number & 0xF0); }
    std::string dissect() const {
      std::stringstream ss;
      ss << "   slot_number\t" << (int)slot_number << std::endl;
      return ss.str();
    }
  } __packed;

  struct ResponsePayload {
    uint8_t slot_password[PWS_PASSWORD_LENGTH];

    bool isValid() const { return true; }
    std::string dissect() const {
      std::stringstream ss;
      ss << " slot_password\t" << (const char*) slot_password << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload,
                      struct ResponsePayload> CommandTransaction;
};

class GetPasswordSafeSlotLogin
    : Command<CommandID::GET_PW_SAFE_SLOT_LOGINNAME> {
 public:
  struct CommandPayload {
    uint8_t slot_number;

    bool isValid() const { return !(slot_number & 0xF0); }
    std::string dissect() const {
      std::stringstream ss;
      ss << "   slot_number\t" << (int)slot_number << std::endl;
      return ss.str();
    }
  } __packed;

  struct ResponsePayload {
    uint8_t slot_login[PWS_LOGINNAME_LENGTH];

    bool isValid() const { return true; }
    std::string dissect() const {
      std::stringstream ss;
      ss << " slot_login\t" << (const char*) slot_login << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload,
                      struct ResponsePayload> CommandTransaction;
};

class SetPasswordSafeSlotData : Command<CommandID::SET_PW_SAFE_SLOT_DATA_1> {
 public:
  struct CommandPayload {
    uint8_t slot_number;
    uint8_t slot_name[PWS_SLOTNAME_LENGTH];
    uint8_t slot_password[PWS_PASSWORD_LENGTH];

    bool isValid() const { return !(slot_number & 0xF0); }
      std::string dissect() const {
          std::stringstream ss;
          ss << " slot_number\t" << (int)slot_number << std::endl;
          ss << " slot_name\t" << (const char*) slot_name << std::endl;
          ss << " slot_password\t" << (const char*) slot_password << std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class SetPasswordSafeSlotData2 : Command<CommandID::SET_PW_SAFE_SLOT_DATA_2> {
 public:
  struct CommandPayload {
    uint8_t slot_number;
    uint8_t slot_login_name[PWS_LOGINNAME_LENGTH];

    bool isValid() const { return !(slot_number & 0xF0); }
      std::string dissect() const {
          std::stringstream ss;
          ss << " slot_number\t" << (int)slot_number << std::endl;
          ss << " slot_login_name\t" << (const char*) slot_login_name << std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class ErasePasswordSafeSlot : Command<CommandID::PW_SAFE_ERASE_SLOT> {
 public:
  struct CommandPayload {
    uint8_t slot_number;

    bool isValid() const { return !(slot_number & 0xF0); }
      std::string dissect() const {
          std::stringstream ss;
          ss << " slot_number\t" << (int)slot_number << std::endl;
          return ss.str();
      }

  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class EnablePasswordSafe : Command<CommandID::PW_SAFE_ENABLE> {
 public:
  struct CommandPayload {
    uint8_t user_password[30];

    bool isValid() const { return true; }
    std::string dissect() const {
      std::stringstream ss;
      ss << " user_password\t" << (const char*)  user_password << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class PasswordSafeInitKey : Command<CommandID::PW_SAFE_INIT_KEY> {
    /**
     * never used in Nitrokey App
     */
 public:
  typedef Transaction<command_id(), struct EmptyPayload, struct EmptyPayload>
      CommandTransaction;
};

class PasswordSafeSendSlotViaHID : Command<CommandID::PW_SAFE_SEND_DATA> {
    /**
     * never used in Nitrokey App
     */
 public:
  struct CommandPayload {
    uint8_t slot_number;
    uint8_t slot_kind;

    bool isValid() const { return !(slot_number & 0xF0); }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

// TODO "Device::passwordSafeSendSlotDataViaHID"

class WriteGeneralConfig : Command<CommandID::WRITE_CONFIG> {
 public:
  struct CommandPayload {
    union{
        uint8_t config[5];
        struct{
            uint8_t numlock;     /** 0-1: HOTP slot number from which the code will be get on double press, other value - function disabled */
            uint8_t capslock;    /** same as numlock */
            uint8_t scrolllock;  /** same as numlock */
            uint8_t enable_user_password;
            uint8_t delete_user_password;
        };
    };
      std::string dissect() const {
          std::stringstream ss;
          ss << "numlock:\t" << (int)numlock << std::endl;
          ss << "capslock:\t" << (int)capslock << std::endl;
          ss << "scrolllock:\t" << (int)scrolllock << std::endl;
          ss << "enable_user_password:\t" << (bool) enable_user_password << std::endl;
          ss << "delete_user_password:\t" << (bool) delete_user_password << std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class FirstAuthenticate : Command<CommandID::FIRST_AUTHENTICATE> {
 public:
  struct CommandPayload {
    uint8_t card_password[25];
    uint8_t temporary_password[25];

    bool isValid() const { return true; }

    std::string dissect() const {
      std::stringstream ss;
      ss << "card_password:\t" << card_password << std::endl;
      ss << "temporary_password:\t" << temporary_password << std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class UserAuthenticate : Command<CommandID::USER_AUTHENTICATE> {
 public:
  struct CommandPayload {
    uint8_t card_password[25];
    uint8_t temporary_password[25];

    bool isValid() const { return true; }
      std::string dissect() const {
          std::stringstream ss;
          ss << "card_password:\t" << card_password << std::endl;
          ss << "temporary_password:\t" << temporary_password << std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class Authorize : Command<CommandID::AUTHORIZE> {
 public:
  struct CommandPayload {
    uint32_t crc_to_authorize;
    uint8_t temporary_password[25];

      std::string dissect() const {
      std::stringstream ss;
      ss << " crc_to_authorize:\t" << std::hex << std::setw(2) << std::setfill('0') << crc_to_authorize<< std::endl;
      ss << " temporary_password:\t" << temporary_password<< std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class UserAuthorize : Command<CommandID::USER_AUTHORIZE> {
 public:
  struct CommandPayload {
    uint32_t crc_to_authorize;
    uint8_t temporary_password[25];
    std::string dissect() const {
      std::stringstream ss;
      ss << " crc_to_authorize:\t" <<  crc_to_authorize<< std::endl;
      ss << " temporary_password:\t" << temporary_password<< std::endl;
      return ss.str();
    }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class UnlockUserPassword : Command<CommandID::UNLOCK_USER_PASSWORD> {
 public:
  struct CommandPayload {
    uint8_t admin_password[25];
    uint8_t user_new_password[25];
      std::string dissect() const {
          std::stringstream ss;
          ss << " admin_password:\t" <<  admin_password<< std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class ChangeUserPin : Command<CommandID::CHANGE_USER_PIN> {
 public:
  struct CommandPayload {
    uint8_t old_pin[25];
    uint8_t new_pin[25];
      std::string dissect() const {
          std::stringstream ss;
          ss << " old_pin:\t" <<  old_pin<< std::endl;
          ss << " new_pin:\t" << new_pin<< std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class IsAESSupported : Command<CommandID::DETECT_SC_AES> {
 public:
  struct CommandPayload {
    uint8_t user_password[20];
      std::string dissect() const {
          std::stringstream ss;
          ss << " user_password:\t" <<  user_password<< std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class ChangeAdminPin : Command<CommandID::CHANGE_ADMIN_PIN> {
 public:
  struct CommandPayload {
    uint8_t old_pin[25];
    uint8_t new_pin[25];
      std::string dissect() const {
          std::stringstream ss;
          ss << " old_pin:\t" <<  old_pin<< std::endl;
          ss << " new_pin:\t" << new_pin<< std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class LockDevice : Command<CommandID::LOCK_DEVICE> {
 public:
  typedef Transaction<command_id(), struct EmptyPayload, struct EmptyPayload>
      CommandTransaction;
};

class FactoryReset : Command<CommandID::FACTORY_RESET> {
 public:
  struct CommandPayload {
    uint8_t admin_password[20];
      std::string dissect() const {
          std::stringstream ss;
          ss << " admin_password:\t" <<  admin_password<< std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;
};

class BuildAESKey : Command<CommandID::NEW_AES_KEY> {
 public:
  struct CommandPayload {
    uint8_t admin_password[20];
      std::string dissect() const {
          std::stringstream ss;
          ss << " admin_password:\t" <<  admin_password<< std::endl;
          return ss.str();
      }
  } __packed;

  typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
      CommandTransaction;

};

}
}
}
#pragma pack (pop)
#endif
