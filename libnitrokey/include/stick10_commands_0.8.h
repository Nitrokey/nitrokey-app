//
// Created by sz on 08.11.16.
//

#ifndef LIBNITROKEY_STICK10_COMMANDS_0_8_H
#define LIBNITROKEY_STICK10_COMMANDS_0_8_H

#include <bitset>
#include <iomanip>
#include <string>
#include <sstream>
#include <stdint.h>
#include "command.h"
#include "device_proto.h"
#include "stick10_commands.h"

namespace nitrokey {
    namespace proto {

/*
 *	Stick10 protocol definition
 */
        namespace stick10_08 {
            using stick10::FirstAuthenticate;
            using stick10::UserAuthenticate;
            using stick10::SetTime;
            using stick10::GetStatus;
            using stick10::BuildAESKey;
            using stick10::ChangeAdminPin;
            using stick10::ChangeUserPin;
            using stick10::EnablePasswordSafe;
            using stick10::ErasePasswordSafeSlot;
            using stick10::FactoryReset;
            using stick10::GetPasswordRetryCount;
            using stick10::GetUserPasswordRetryCount;
            using stick10::GetPasswordSafeSlotLogin;
            using stick10::GetPasswordSafeSlotName;
            using stick10::GetPasswordSafeSlotPassword;
            using stick10::GetPasswordSafeSlotStatus;
            using stick10::GetSlotName;
            using stick10::IsAESSupported;
            using stick10::LockDevice;
            using stick10::PasswordSafeInitKey;
            using stick10::PasswordSafeSendSlotViaHID;
            using stick10::SetPasswordSafeSlotData;
            using stick10::SetPasswordSafeSlotData2;
            using stick10::UnlockUserPassword;
            using stick10::ReadSlot;

            class EraseSlot : Command<CommandID::ERASE_SLOT> {
            public:
                struct CommandPayload {
                    uint8_t slot_number;
                    uint8_t temporary_admin_password[25];

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

            class SendOTPData : Command<CommandID::SEND_OTP_DATA> {
                //admin auth
            public:
                struct CommandPayload {
                    uint8_t temporary_admin_password[25];
                    uint8_t type; //S-secret, N-name
                    uint8_t id; //multiple reports for values longer than 30 bytes
                    uint8_t data[30]; //data, does not need null termination

                    bool isValid() const { return true; }

                    void setTypeName(){
                      type = 'N';
                    }
                    void setTypeSecret(){
                      type = 'S';
                    }

                    std::string dissect() const {
                      std::stringstream ss;
                      ss << "temporary_admin_password:\t" << temporary_admin_password << std::endl;
                      ss << "type:\t" << type << std::endl;
                      ss << "id:\t" << (int)id << std::endl;
                      ss << "data:" << std::endl
                         << ::nitrokey::misc::hexdump((const char *) (&data), sizeof data);
                      return ss.str();
                    }
                } __packed;


                struct ResponsePayload {
                    union {
                        uint8_t data[40];
                    } __packed;

                    bool isValid() const { return true; }
                    std::string dissect() const {
                      std::stringstream ss;
                      ss << "data:" << std::endl
                         << ::nitrokey::misc::hexdump((const char *) (&data), sizeof data);
                      return ss.str();
                    }
                } __packed;


                typedef Transaction<command_id(), struct CommandPayload, struct ResponsePayload>
                    CommandTransaction;
            };

            class WriteToOTPSlot : Command<CommandID::WRITE_TO_SLOT> {
                //admin auth
            public:
                struct CommandPayload {
                    uint8_t temporary_admin_password[25];
                    uint8_t slot_number;
                    union {
                        uint64_t slot_counter_or_interval;
                        uint8_t slot_counter_s[8];
                    } __packed;
                    union {
                        uint8_t _slot_config;
                        struct {
                            bool use_8_digits   : 1;
                            bool use_enter      : 1;
                            bool use_tokenID    : 1;
                        };
                    };
                    union {
                        uint8_t slot_token_id[13]; /** OATH Token Identifier */
                        struct { /** @see https://openauthentication.org/token-specs/ */
                            uint8_t omp[2];
                            uint8_t tt[2];
                            uint8_t mui[8];
                            uint8_t keyboard_layout; //disabled feature in nitroapp as of 20160805
                        } slot_token_fields;
                    };

                    bool isValid() const { return true; }

                    std::string dissect() const {
                      std::stringstream ss;
                      ss << "temporary_admin_password:\t" << temporary_admin_password << std::endl;
                      ss << "slot_config:\t" << std::bitset<8>((int) _slot_config) << std::endl;
                      ss << "\tuse_8_digits(0):\t" << use_8_digits << std::endl;
                      ss << "\tuse_enter(1):\t" << use_enter << std::endl;
                      ss << "\tuse_tokenID(2):\t" << use_tokenID << std::endl;
                      ss << "slot_number:\t" << (int) (slot_number) << std::endl;
                      ss << "slot_counter_or_interval:\t[" << (int) slot_counter_or_interval << "]\t"
                         << ::nitrokey::misc::hexdump((const char *) (&slot_counter_or_interval), sizeof slot_counter_or_interval, false);

                      ss << "slot_token_id:\t";
                      for (auto i : slot_token_id)
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int) i << " ";
                      ss << std::endl;

                      return ss.str();
                    }
                } __packed;

                typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
                    CommandTransaction;
            };

            class GetHOTP : Command<CommandID::GET_CODE> {
            public:
                struct CommandPayload {
                    uint8_t slot_number;
                    struct {
                        uint64_t challenge; //@unused
                        uint64_t last_totp_time; //@unused
                        uint8_t last_interval; //@unused
                    } __packed _unused;
                    uint8_t temporary_user_password[25];

                    bool isValid() const { return (slot_number & 0xF0); }
                    std::string dissect() const {
                      std::stringstream ss;
                      ss << "temporary_user_password:\t" << temporary_user_password << std::endl;
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


            class GetTOTP : Command<CommandID::GET_CODE> {
                //user auth
            public:
                struct CommandPayload {
                    uint8_t slot_number;
                    uint64_t challenge; //@unused
                    uint64_t last_totp_time; //@unused
                    uint8_t last_interval; //@unused
                    uint8_t temporary_user_password[25];

                    bool isValid() const { return !(slot_number & 0xF0); }
                    std::string dissect() const {
                      std::stringstream ss;
                      ss << "temporary_user_password:\t" << temporary_user_password << std::endl;
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


            class WriteGeneralConfig : Command<CommandID::WRITE_CONFIG> {
                //admin auth
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
                    uint8_t temporary_admin_password[25];

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
        }
    }
}
#endif //LIBNITROKEY_STICK10_COMMANDS_0_8_H
