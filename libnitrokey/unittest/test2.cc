
static const char *const default_admin_pin = "12345678";
static const char *const default_user_pin = "123456";

#include "catch.hpp"

#include <iostream>
#include <string.h>
#include <NitrokeyManager.h>
#include "device_proto.h"
#include "log.h"
//#include "stick10_commands.h"
#include "stick20_commands.h"

using namespace std;
using namespace nitrokey::device;
using namespace nitrokey::proto;
using namespace nitrokey::proto::stick20;
using namespace nitrokey::log;
using namespace nitrokey::misc;

#include <memory>

template<typename CMDTYPE>
void execute_password_command(std::shared_ptr<Device> stick, const char *password, const char kind = 'P') {
  auto p = get_payload<CMDTYPE>();
  if (kind == 'P'){
    p.set_kind_user();
  } else {
    p.set_kind_admin();
  }
  strcpyT(p.password, password);
  CMDTYPE::CommandTransaction::run(stick, p);
  this_thread::sleep_for(1000ms);
}

/**
 *  fail on purpose (will result in failed test)
 *  disable from running unwillingly
 */
void SKIP_TEST() {
  CAPTURE("Failing current test to SKIP it");
  REQUIRE(false);
}


TEST_CASE("long operation test", "[test_long]") {
  SKIP_TEST();

  auto stick = make_shared<Stick20>();
  bool connected = stick->connect();
  REQUIRE(connected == true);
  Log::instance().set_loglevel(Loglevel::DEBUG);
  try{
    auto p = get_payload<FillSDCardWithRandomChars>();
    p.set_defaults();
    strcpyT(p.admin_pin, default_admin_pin);
    FillSDCardWithRandomChars::CommandTransaction::run(stick, p);
    this_thread::sleep_for(1000ms);

    CHECK(false);
  }
  catch (LongOperationInProgressException &progressException){
    CHECK(true);
  }


  for (int i = 0; i < 30; ++i) {
    try {
      stick10::GetStatus::CommandTransaction::run(stick);
    }
    catch (LongOperationInProgressException &progressException){
      CHECK((int)progressException.progress_bar_value>=0);
      CAPTURE((int)progressException.progress_bar_value);
      this_thread::sleep_for(2000ms);
    }

  }

}


#include "test_command_ids_header.h"

TEST_CASE("test device commands ids", "[fast]") {

//  REQUIRE(STICK20_CMD_START_VALUE == static_cast<uint8_t>(CommandID::START_VALUE));
  REQUIRE(STICK20_CMD_ENABLE_CRYPTED_PARI == static_cast<uint8_t>(CommandID::ENABLE_CRYPTED_PARI));
  REQUIRE(STICK20_CMD_DISABLE_CRYPTED_PARI == static_cast<uint8_t>(CommandID::DISABLE_CRYPTED_PARI));
  REQUIRE(STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI == static_cast<uint8_t>(CommandID::ENABLE_HIDDEN_CRYPTED_PARI));
  REQUIRE(STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI == static_cast<uint8_t>(CommandID::DISABLE_HIDDEN_CRYPTED_PARI));
  REQUIRE(STICK20_CMD_ENABLE_FIRMWARE_UPDATE == static_cast<uint8_t>(CommandID::ENABLE_FIRMWARE_UPDATE));
  REQUIRE(STICK20_CMD_EXPORT_FIRMWARE_TO_FILE == static_cast<uint8_t>(CommandID::EXPORT_FIRMWARE_TO_FILE));
  REQUIRE(STICK20_CMD_GENERATE_NEW_KEYS == static_cast<uint8_t>(CommandID::GENERATE_NEW_KEYS));
  REQUIRE(STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS == static_cast<uint8_t>(CommandID::FILL_SD_CARD_WITH_RANDOM_CHARS));

  REQUIRE(STICK20_CMD_WRITE_STATUS_DATA == static_cast<uint8_t>(CommandID::WRITE_STATUS_DATA));
  REQUIRE(STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN == static_cast<uint8_t>(CommandID::ENABLE_READONLY_UNCRYPTED_LUN));
  REQUIRE(STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN == static_cast<uint8_t>(CommandID::ENABLE_READWRITE_UNCRYPTED_LUN));

  REQUIRE(STICK20_CMD_SEND_PASSWORD_MATRIX == static_cast<uint8_t>(CommandID::SEND_PASSWORD_MATRIX));
  REQUIRE(STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA == static_cast<uint8_t>(CommandID::SEND_PASSWORD_MATRIX_PINDATA));
  REQUIRE(STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP == static_cast<uint8_t>(CommandID::SEND_PASSWORD_MATRIX_SETUP));

  REQUIRE(STICK20_CMD_GET_DEVICE_STATUS == static_cast<uint8_t>(CommandID::GET_DEVICE_STATUS));
  REQUIRE(STICK20_CMD_SEND_DEVICE_STATUS == static_cast<uint8_t>(CommandID::SEND_DEVICE_STATUS));

  REQUIRE(STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD == static_cast<uint8_t>(CommandID::SEND_HIDDEN_VOLUME_PASSWORD));
  REQUIRE(STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP == static_cast<uint8_t>(CommandID::SEND_HIDDEN_VOLUME_SETUP));
  REQUIRE(STICK20_CMD_SEND_PASSWORD == static_cast<uint8_t>(CommandID::SEND_PASSWORD));
  REQUIRE(STICK20_CMD_SEND_NEW_PASSWORD == static_cast<uint8_t>(CommandID::SEND_NEW_PASSWORD));
  REQUIRE(STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND == static_cast<uint8_t>(CommandID::CLEAR_NEW_SD_CARD_FOUND));

  REQUIRE(STICK20_CMD_SEND_STARTUP == static_cast<uint8_t>(CommandID::SEND_STARTUP));
  REQUIRE(STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED == static_cast<uint8_t>(CommandID::SEND_CLEAR_STICK_KEYS_NOT_INITIATED));
  REQUIRE(STICK20_CMD_SEND_LOCK_STICK_HARDWARE == static_cast<uint8_t>(CommandID::SEND_LOCK_STICK_HARDWARE));

  REQUIRE(STICK20_CMD_PRODUCTION_TEST == static_cast<uint8_t>(CommandID::PRODUCTION_TEST));
  REQUIRE(STICK20_CMD_SEND_DEBUG_DATA == static_cast<uint8_t>(CommandID::SEND_DEBUG_DATA));

  REQUIRE(STICK20_CMD_CHANGE_UPDATE_PIN == static_cast<uint8_t>(CommandID::CHANGE_UPDATE_PIN));

}

TEST_CASE("test device internal status with various commands", "[fast]") {
  auto stick = make_shared<Stick20>();
  bool connected = stick->connect();
  REQUIRE(connected == true);

  Log::instance().set_loglevel(Loglevel::DEBUG);
  auto p = get_payload<stick20::SendStartup>();
  p.set_defaults();
  auto device_status = stick20::SendStartup::CommandTransaction::run(stick, p);
  REQUIRE(device_status.data().AdminPwRetryCount == 3);
  REQUIRE(device_status.data().UserPwRetryCount == 3);
  REQUIRE(device_status.data().ActiveSmartCardID_u32 != 0);

  auto production_status = stick20::ProductionTest::CommandTransaction::run(stick);
  REQUIRE(production_status.data().SD_Card_Size_u8 == 8);
  REQUIRE(production_status.data().SD_CardID_u32 != 0);

  auto sdcard_occupancy = stick20::GetSDCardOccupancy::CommandTransaction::run(stick);
  REQUIRE((int) sdcard_occupancy.data().ReadLevelMin >= 0);
  REQUIRE((int) sdcard_occupancy.data().ReadLevelMax <= 100);
  REQUIRE((int) sdcard_occupancy.data().WriteLevelMin >= 0);
  REQUIRE((int) sdcard_occupancy.data().WriteLevelMax <= 100);
}

TEST_CASE("setup hidden volume test", "[hidden]") {
  auto stick = make_shared<Stick20>();
  bool connected = stick->connect();
  REQUIRE(connected == true);
  Log::instance().set_loglevel(Loglevel::DEBUG);
  stick10::LockDevice::CommandTransaction::run(stick);
  this_thread::sleep_for(2000ms);

  auto user_pin = default_user_pin;
  execute_password_command<EnableEncryptedPartition>(stick, user_pin);

  auto p = get_payload<stick20::SetupHiddenVolume>();
  p.SlotNr_u8 = 0;
  p.StartBlockPercent_u8 = 70;
  p.EndBlockPercent_u8 = 90;
  auto hidden_volume_password = "123123123";
  strcpyT(p.HiddenVolumePassword_au8, hidden_volume_password);
  stick20::SetupHiddenVolume::CommandTransaction::run(stick, p);
  this_thread::sleep_for(2000ms);

  execute_password_command<EnableHiddenEncryptedPartition>(stick, hidden_volume_password);
}

TEST_CASE("setup multiple hidden volumes", "[hidden]") {
  auto stick = make_shared<Stick20>();
  bool connected = stick->connect();
  REQUIRE(connected == true);
  Log::instance().set_loglevel(Loglevel::DEBUG);

  auto user_pin = default_user_pin;
  stick10::LockDevice::CommandTransaction::run(stick);
  this_thread::sleep_for(2000ms);
  execute_password_command<EnableEncryptedPartition>(stick, user_pin);

  constexpr int volume_count = 4;
  for (int i = 0; i < volume_count; ++i) {
    auto p = get_payload<stick20::SetupHiddenVolume>();
    p.SlotNr_u8 = i;
    p.StartBlockPercent_u8 = 20 + 10*i;
    p.EndBlockPercent_u8 = p.StartBlockPercent_u8+i+1;
    auto hidden_volume_password = std::string("123123123")+std::to_string(i);
    strcpyT(p.HiddenVolumePassword_au8, hidden_volume_password.c_str());
    stick20::SetupHiddenVolume::CommandTransaction::run(stick, p);
    this_thread::sleep_for(2000ms);
  }


  for (int i = 0; i < volume_count; ++i) {
    execute_password_command<EnableEncryptedPartition>(stick, user_pin);
    auto hidden_volume_password = std::string("123123123")+std::to_string(i);
    execute_password_command<EnableHiddenEncryptedPartition>(stick, hidden_volume_password.c_str());
    this_thread::sleep_for(2000ms);
  }
}


//in case of a bug this could change update PIN to some unexpected value
// - please save log with packet dump if this test will not pass
TEST_CASE("update password change", "[dangerous]") {
  SKIP_TEST();

  auto stick = make_shared<Stick20>();
  bool connected = stick->connect();
  REQUIRE(connected == true);
  Log::instance().set_loglevel(Loglevel::DEBUG);

  auto pass1 = default_admin_pin;
  auto pass2 = "12345678901234567890";

  auto data = {
      make_pair(pass1, pass2),
      make_pair(pass2, pass1),
  };
  for (auto &&  password: data) {
    auto p = get_payload<stick20::ChangeUpdatePassword>();
    strcpyT(p.current_update_password, password.first);
    strcpyT(p.new_update_password, password.second);
    stick20::ChangeUpdatePassword::CommandTransaction::run(stick, p);
  }
}

TEST_CASE("general test", "[test]") {
  auto stick = make_shared<Stick20>();
  bool connected = stick->connect();
  REQUIRE(connected == true);

  Log::instance().set_loglevel(Loglevel::DEBUG);

  stick10::LockDevice::CommandTransaction::run(stick);
//  execute_password_command<EnableEncryptedPartition>(stick, "123456");
//  execute_password_command<DisableEncryptedPartition>(stick, "123456");
//  execute_password_command<DisableHiddenEncryptedPartition>(stick, "123123123");

  execute_password_command<SendSetReadonlyToUncryptedVolume>(stick, default_user_pin);
  execute_password_command<SendSetReadwriteToUncryptedVolume>(stick, default_user_pin);
  execute_password_command<SendClearNewSdCardFound>(stick, default_admin_pin, 'A');
  stick20::GetDeviceStatus::CommandTransaction::run(stick);
  this_thread::sleep_for(1000ms);
//  execute_password_command<LockFirmware>(stick, "123123123"); //CAUTION
//  execute_password_command<EnableFirmwareUpdate>(stick, "123123123"); //CAUTION FIRMWARE PIN

  execute_password_command<ExportFirmware>(stick, "12345678", 'A');
//  execute_password_command<FillSDCardWithRandomChars>(stick, "12345678", 'A');

  stick10::LockDevice::CommandTransaction::run(stick);
}
