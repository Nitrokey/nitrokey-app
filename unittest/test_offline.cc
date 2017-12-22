#include "catch.hpp"
#include <NitrokeyManager.h>
#include <memory>
#include "../NK_C_API.h"

using namespace nitrokey::proto;
using namespace nitrokey::device;

using namespace std;
using namespace nitrokey;

//This test suite assumes no Pro or Storage devices are connected

TEST_CASE("Return false on no device connected", "[fast]") {
  INFO("This test case assumes no Pro or Storage devices are connected");
  auto stick = make_shared<Stick20>();
  bool connected = true;
  REQUIRE_NOTHROW(connected = stick->connect());
  REQUIRE_FALSE(connected);

  auto stick_pro = make_shared<Stick10>();
  REQUIRE_NOTHROW(connected = stick_pro->connect());
  REQUIRE_FALSE(connected);


  auto i = NitrokeyManager::instance();
  REQUIRE_NOTHROW(connected = i->connect());
  REQUIRE_FALSE(connected);
  REQUIRE_FALSE(i->is_connected());
  REQUIRE_FALSE(i->disconnect());
  REQUIRE_FALSE(i->could_current_device_be_enumerated());


  int C_connected = 1;
  REQUIRE_NOTHROW(C_connected = NK_login_auto());
  REQUIRE(0 == C_connected);
}

TEST_CASE("Test C++ side behaviour in offline", "[fast]") {
  auto i = NitrokeyManager::instance();

  string serial_number;
  REQUIRE_NOTHROW (serial_number = i->get_serial_number());
  REQUIRE(serial_number.empty());

  REQUIRE_THROWS_AS(
    i->get_status(), DeviceNotConnected
  );

  REQUIRE_THROWS_AS(
      i->get_HOTP_code(0xFF, ""), InvalidSlotException
  );

  REQUIRE_THROWS_AS(
      i->get_TOTP_code(0xFF, ""), InvalidSlotException
  );

  REQUIRE_THROWS_AS(
      i->erase_hotp_slot(0xFF, ""), InvalidSlotException
  );

  REQUIRE_THROWS_AS(
      i->erase_totp_slot(0xFF, ""), InvalidSlotException
  );

  REQUIRE_THROWS_AS(
      i->get_totp_slot_name(0xFF), InvalidSlotException
  );

  REQUIRE_THROWS_AS(
      i->get_hotp_slot_name(0xFF), InvalidSlotException
  );

  REQUIRE_THROWS_AS(
      i->first_authenticate("123123", "123123"), DeviceNotConnected
  );

  REQUIRE_THROWS_AS(
      i->get_connected_device_model(), DeviceNotConnected
  );

  REQUIRE_THROWS_AS(
      i->clear_new_sd_card_warning("123123"), DeviceNotConnected
  );

}


TEST_CASE("Test helper function - hex_string_to_byte", "[fast]") {
  using namespace nitrokey::misc;
  std::vector<uint8_t> v;
  REQUIRE_NOTHROW(v = hex_string_to_byte("00112233445566"));
  const uint8_t test_data[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  REQUIRE(v.size() == sizeof(test_data));
  for (int i = 0; i < v.size(); ++i) {
    INFO("Position i: " << i);
    REQUIRE(v.data()[i] == test_data[i]);
  }
}

#include "test_command_ids_header.h"
TEST_CASE("Test device commands ids", "[fast]") {
// Make sure CommandID values are in sync with firmware's header

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
