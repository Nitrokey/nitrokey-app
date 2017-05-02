#ifndef STICK20_COMMANDS_H
#define STICK20_COMMANDS_H



#include <stdint.h>
#include "command.h"
#include <string>
#include <sstream>
#include "device_proto.h"

#pragma pack (push,1)

namespace nitrokey {
    namespace proto {

/*
*	STICK20 protocol command ids
*	a superset (almost) of STICK10
*/
#define print_to_ss(x) ( ss << " " << (#x) <<":\t" << (x) << std::endl );
        namespace stick20 {

            class ChangeAdminUserPin20Current :
                public PasswordCommand<CommandID::SEND_PASSWORD, PasswordKind::Admin> {};
            class ChangeAdminUserPin20New :
                public PasswordCommand<CommandID::SEND_NEW_PASSWORD, PasswordKind::Admin> {};
            class UnlockUserPin :
                public PasswordCommand<CommandID::UNLOCK_USER_PASSWORD, PasswordKind::Admin> {};

            class EnableEncryptedPartition : public PasswordCommand<CommandID::ENABLE_CRYPTED_PARI> {};
            class EnableHiddenEncryptedPartition : public PasswordCommand<CommandID::ENABLE_HIDDEN_CRYPTED_PARI> {};

            //FIXME the volume disabling commands do not need password
            class DisableEncryptedPartition : public PasswordCommand<CommandID::DISABLE_CRYPTED_PARI> {};
            class DisableHiddenEncryptedPartition : public PasswordCommand<CommandID::DISABLE_HIDDEN_CRYPTED_PARI> {};

            class EnableFirmwareUpdate : public PasswordCommand<CommandID::ENABLE_FIRMWARE_UPDATE> {};

            class ChangeUpdatePassword : Command<CommandID::CHANGE_UPDATE_PIN> {
            public:
                struct CommandPayload {
                    uint8_t __gap;
                    uint8_t current_update_password[20];
                    uint8_t __gap2;
                    uint8_t new_update_password[20];
                    std::string dissect() const {
                      std::stringstream ss;
                      print_to_ss( current_update_password );
                      print_to_ss( new_update_password );
                      return ss.str();
                    }
                };

                typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
                    CommandTransaction;
            };

            class ExportFirmware : public PasswordCommand<CommandID::EXPORT_FIRMWARE_TO_FILE> {};

            class CreateNewKeys :
                public PasswordCommand<CommandID::GENERATE_NEW_KEYS, PasswordKind::AdminPrefixed, 30> {};


            class FillSDCardWithRandomChars : Command<CommandID::FILL_SD_CARD_WITH_RANDOM_CHARS> {
            public:
                enum class ChosenVolumes : uint8_t {
                    all_volumes = 0,
                    encrypted_volume = 1
                };

                struct CommandPayload {
                    uint8_t volume_flag;
                    uint8_t kind;
                    uint8_t admin_pin[20];

                    std::string dissect() const {
                      std::stringstream ss;
                      print_to_ss( (int) volume_flag );
                      print_to_ss( kind );
                      print_to_ss(admin_pin);
                      return ss.str();
                    }
                    void set_kind_user() {
                      kind = (uint8_t) 'P';
                    }
                    void set_defaults(){
                      set_kind_user();
                      volume_flag = static_cast<uint8_t>(ChosenVolumes::encrypted_volume);
                    }

                } __packed;

                typedef Transaction<Command<CommandID::FILL_SD_CARD_WITH_RANDOM_CHARS>::command_id(),
                    struct CommandPayload, struct EmptyPayload>
                    CommandTransaction;
            };

            namespace StorageCommandResponsePayload{
                using namespace DeviceResponseConstants;
              static constexpr auto padding_size =
                  storage_data_absolute_address - header_size;
              struct TransmissionData{
                  uint8_t _padding[padding_size];

                  uint8_t SendCounter_u8;
                  uint8_t SendDataType_u8;
                  uint8_t FollowBytesFlag_u8;
                  uint8_t SendSize_u8;

                  std::string dissect() const {
                    std::stringstream ss;
                    ss << "_padding:" << std::endl
                       << ::nitrokey::misc::hexdump((const char *) (_padding),
                                                    sizeof _padding);
                    print_to_ss((int) SendCounter_u8);
                    print_to_ss((int) SendDataType_u8);
                    print_to_ss((int) FollowBytesFlag_u8);
                    print_to_ss((int) SendSize_u8);
                    return ss.str();
                  }

              } __packed;
            }

            namespace DeviceConfigurationResponsePacket{

                struct ResponsePayload {
                    StorageCommandResponsePayload::TransmissionData transmission_data;

                    uint16_t MagicNumber_StickConfig_u16;
                  /**
                   * READ_WRITE_ACTIVE = ReadWriteFlagUncryptedVolume_u8 == 0;
                   */
                    uint8_t ReadWriteFlagUncryptedVolume_u8;
                    uint8_t ReadWriteFlagCryptedVolume_u8;

                    union{
                    uint8_t VersionInfo_au8[4];
                        struct {
                            uint8_t __reserved;
                            uint8_t minor;
                            uint8_t __reserved2;
                            uint8_t major;
                        } __packed versionInfo;
                    } __packed;

                    uint8_t ReadWriteFlagHiddenVolume_u8;
                    uint8_t FirmwareLocked_u8;

                    union{
                      uint8_t NewSDCardFound_u8;
                      struct {
                        bool NewCard :1;
                        uint8_t Counter :7;
                      } __packed NewSDCardFound_st;
                    } __packed;

                    /**
                     * SD card FILLED with random chars
                     */
                    uint8_t SDFillWithRandomChars_u8;
                    uint32_t ActiveSD_CardID_u32;
                    union{
                      uint8_t VolumeActiceFlag_u8;
                        struct {
                            bool unencrypted :1;
                            bool encrypted :1;
                            bool hidden :1;
                        } __packed VolumeActiceFlag_st;
                    } __packed;
                    uint8_t NewSmartCardFound_u8;
                    uint8_t UserPwRetryCount;
                    uint8_t AdminPwRetryCount;
                    uint32_t ActiveSmartCardID_u32;
                    uint8_t StickKeysNotInitiated;

                    bool isValid() const { return true; }

                    std::string dissect() const {
                      std::stringstream ss;

                      print_to_ss(transmission_data.dissect());
                      print_to_ss( MagicNumber_StickConfig_u16 );
                      print_to_ss((int) ReadWriteFlagUncryptedVolume_u8 );
                      print_to_ss((int) ReadWriteFlagCryptedVolume_u8 );
                      print_to_ss((int) VersionInfo_au8[1] );
                      print_to_ss((int) VersionInfo_au8[3] );
                      print_to_ss((int) ReadWriteFlagHiddenVolume_u8 );
                      print_to_ss((int) FirmwareLocked_u8 );
                      print_to_ss((int) NewSDCardFound_u8 );
                      print_to_ss((int) SDFillWithRandomChars_u8 );
                      print_to_ss( ActiveSD_CardID_u32 );
                      print_to_ss((int) VolumeActiceFlag_u8 );
                      print_to_ss((int) NewSmartCardFound_u8 );
                      print_to_ss((int) UserPwRetryCount );
                      print_to_ss((int) AdminPwRetryCount );
                      print_to_ss( ActiveSmartCardID_u32 );
                      print_to_ss((int) StickKeysNotInitiated );

                      return ss.str();
                    }
                } __packed;
            }

            class SendStartup : Command<CommandID::SEND_STARTUP> {
            public:
                struct CommandPayload {
                    uint64_t localtime;  // POSIX seconds from epoch start, supports until year 2106
                    std::string dissect() const {
                      std::stringstream ss;
                      print_to_ss( localtime );
                      return ss.str();
                    }
                    void set_defaults(){
                      localtime =
                          std::chrono::duration_cast<std::chrono::seconds> (
                              std::chrono::system_clock::now().time_since_epoch()).count();
                    }
                }__packed;

                using ResponsePayload = DeviceConfigurationResponsePacket::ResponsePayload;

                typedef Transaction<command_id(), struct CommandPayload, ResponsePayload>
                    CommandTransaction;
            };


// TODO fix original nomenclature
            class SendSetReadonlyToUncryptedVolume : public PasswordCommand<CommandID::ENABLE_READONLY_UNCRYPTED_LUN> {};
            class SendSetReadwriteToUncryptedVolume : public PasswordCommand<CommandID::ENABLE_READWRITE_UNCRYPTED_LUN> {};
            class SendClearNewSdCardFound : public PasswordCommand<CommandID::CLEAR_NEW_SD_CARD_FOUND> {};

            class GetDeviceStatus : Command<CommandID::GET_DEVICE_STATUS> {
            public:
                using ResponsePayload = DeviceConfigurationResponsePacket::ResponsePayload;

                typedef Transaction<command_id(), struct EmptyPayload, ResponsePayload>
                    CommandTransaction;
            };

            class GetSDCardOccupancy : Command<CommandID::SD_CARD_HIGH_WATERMARK> {
            public:
                struct ResponsePayload {
                    uint8_t WriteLevelMin;
                    uint8_t WriteLevelMax;
                    uint8_t ReadLevelMin;
                    uint8_t ReadLevelMax;
                    std::string dissect() const {
                      std::stringstream ss;
                      print_to_ss((int) WriteLevelMin);
                      print_to_ss((int) WriteLevelMax);
                      print_to_ss((int) ReadLevelMin);
                      print_to_ss((int) ReadLevelMax);
                      return ss.str();
                    }
                } __packed;

                typedef Transaction<command_id(), struct EmptyPayload, struct ResponsePayload>
                    CommandTransaction;
            };


            class SetupHiddenVolume : Command<CommandID::SEND_HIDDEN_VOLUME_SETUP> {
            public:
                constexpr static int MAX_HIDDEN_VOLUME_PASSWORD_SIZE = 20;
                struct CommandPayload {
                    uint8_t SlotNr_u8;
                    uint8_t StartBlockPercent_u8;
                    uint8_t EndBlockPercent_u8;
                    uint8_t HiddenVolumePassword_au8[MAX_HIDDEN_VOLUME_PASSWORD_SIZE];
                    std::string dissect() const {
                      std::stringstream ss;
                      print_to_ss((int) SlotNr_u8);
                      print_to_ss((int) StartBlockPercent_u8);
                      print_to_ss((int) EndBlockPercent_u8);
                      print_to_ss(HiddenVolumePassword_au8);
                      return ss.str();
                    }
                } __packed;

                typedef Transaction<command_id(), struct CommandPayload, struct EmptyPayload>
                    CommandTransaction;
            };


//disable this command for now
//            class LockFirmware : public PasswordCommand<CommandID::SEND_LOCK_STICK_HARDWARE> {};

            class ProductionTest : Command<CommandID::PRODUCTION_TEST> {
            public:
                struct ResponsePayload {

                    StorageCommandResponsePayload::TransmissionData transmission_data;

                    uint8_t FirmwareVersion_au8[2];        // 2 byte // 2
                    uint8_t FirmwareVersionInternal_u8;    // 1 byte // 3
                    uint8_t SD_Card_Size_u8;               // 1 byte // 4
                    uint32_t CPU_CardID_u32;                     // 4 byte // 8
                    uint32_t SmartCardID_u32;                    // 4 byte // 12
                    uint32_t SD_CardID_u32;                      // 4 byte // 16
                    uint8_t SC_UserPwRetryCount;           // User PIN retry count 1 byte  // 17
                    uint8_t SC_AdminPwRetryCount;          // Admin PIN retry count 1 byte // 18
                    uint8_t SD_Card_ManufacturingYear_u8;  // 1 byte // 19
                    uint8_t SD_Card_ManufacturingMonth_u8; // 1 byte // 20
                    uint16_t SD_Card_OEM_u16;              // 2 byte // 22
                    uint16_t SD_WriteSpeed_u16;            // in kbyte / sec 2 byte // 24
                    uint8_t SD_Card_Manufacturer_u8;       // 1 byte // 25

                    bool isValid() const { return true; }

                    std::string dissect() const {
                      std::stringstream ss;

                      print_to_ss(transmission_data.dissect());
                      print_to_ss((int) FirmwareVersion_au8[0]);
                      print_to_ss((int) FirmwareVersion_au8[1]);
                      print_to_ss((int) FirmwareVersionInternal_u8);
                      print_to_ss((int) SD_Card_Size_u8);
                      print_to_ss( CPU_CardID_u32);
                      print_to_ss( SmartCardID_u32);
                      print_to_ss( SD_CardID_u32);
                      print_to_ss((int) SC_UserPwRetryCount);
                      print_to_ss((int) SC_AdminPwRetryCount);
                      print_to_ss((int) SD_Card_ManufacturingYear_u8);
                      print_to_ss((int) SD_Card_ManufacturingMonth_u8);
                      print_to_ss( SD_Card_OEM_u16);
                      print_to_ss( SD_WriteSpeed_u16);
                      print_to_ss((int) SD_Card_Manufacturer_u8);
                      return ss.str();
                    }

                  } __packed;

                typedef Transaction<command_id(), struct EmptyPayload, struct ResponsePayload>
                    CommandTransaction;
            };

        }
    }
}

#undef print_to_ss
#pragma pack (pop)

#endif
