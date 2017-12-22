#include "catch.hpp"
#include <iostream>
#include "device_proto.h"
#include "log.h"
#include "stick10_commands.h"
#include <cstdlib>
#include "misc.h"

using namespace std;
using namespace nitrokey::device;
using namespace nitrokey::proto::stick10;
using namespace nitrokey::log;
using namespace nitrokey::misc;

void hexStringToByte(uint8_t data[], const char* hexString){
  REQUIRE(strlen(hexString)%2==0);
    char buf[2];
    for(int i=0; i<strlen(hexString); i++){
        buf[i%2] = hexString[i];
        if (i%2==1){
            data[i/2] = strtoul(buf, NULL, 16) & 0xFF;
        }
    }
};

TEST_CASE("test secret", "[functions]") {
    uint8_t slot_secret[21];
    slot_secret[20] = 0;
    const char* secretHex = "3132333435363738393031323334353637383930";
    hexStringToByte(slot_secret, secretHex);
    CAPTURE(slot_secret);
    REQUIRE(strcmp("12345678901234567890",reinterpret_cast<char *>(slot_secret) ) == 0 );
}

TEST_CASE("Test HOTP codes according to RFC", "[HOTP]") {
    std::shared_ptr<Stick10> stick = make_shared<Stick10>();
    bool connected = stick->connect();

  REQUIRE(connected == true);

  Log::instance().set_loglevel(Loglevel::DEBUG);

  auto resp = GetStatus::CommandTransaction::run(stick);

  const char * temporary_password = "123456789012345678901234";
  {
      auto authreq = get_payload<FirstAuthenticate>();
      strcpy((char *)(authreq.card_password), "12345678");
      strcpy((char *)(authreq.temporary_password), temporary_password);
      FirstAuthenticate::CommandTransaction::run(stick, authreq);
  }

  //test according to https://tools.ietf.org/html/rfc4226#page-32
  {
    auto hwrite = get_payload<WriteToHOTPSlot>();
    hwrite.slot_number = 0x10;
    strcpy(reinterpret_cast<char *>(hwrite.slot_name), "rfc4226_lib");
    //strcpy(reinterpret_cast<char *>(hwrite.slot_secret), "");
    const char* secretHex = "3132333435363738393031323334353637383930";
    hexStringToByte(hwrite.slot_secret, secretHex);

    //hwrite.slot_config; //TODO check various configs in separate test cases
    //strcpy(reinterpret_cast<char *>(hwrite.slot_token_id), "");
    //strcpy(reinterpret_cast<char *>(hwrite.slot_counter), "");

    //authorize writehotp first
    {
        auto auth = get_payload<Authorize>();
        strcpy((char *)(auth.temporary_password), temporary_password);
        auth.crc_to_authorize = WriteToHOTPSlot::CommandTransaction::getCRC(hwrite);
        Authorize::CommandTransaction::run(stick, auth);
  }

    //run hotp command
    WriteToHOTPSlot::CommandTransaction::run(stick, hwrite);

    uint32_t codes[] = {
            755224, 287082, 359152, 969429, 338314,
            254676, 287922, 162583, 399871, 520489
    };

    for( auto code: codes){
        auto gh = get_payload<GetHOTP>();
        gh.slot_number =  0x10;
        auto resp = GetHOTP::CommandTransaction::run(stick, gh);
        REQUIRE( resp.data().code == code);
    }
    //checking slot programmed before with nitro-app
    /*
    for( auto code: codes){
        GetHOTP::CommandTransaction::CommandPayload gh;
        gh.slot_number =  0x12;
        auto resp = GetHOTP::CommandTransaction::run(stick, gh);
        REQUIRE( resp.code == code);
    }
    */
  }


  stick->disconnect();
}
