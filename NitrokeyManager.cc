#include <cstring>
#include <iostream>
#include "include/NitrokeyManager.h"
#include "include/LibraryException.h"
#include <algorithm>
#include <unordered_map>
#include <stick20_commands.h>
#include "include/misc.h"
#include <mutex>
#include "include/cxx_semantics.h"
#include <functional>

namespace nitrokey{

    std::mutex mex_dev_com_manager;

#ifdef __WIN32
#pragma message "Using own strndup"
char * strndup(const char* str, size_t maxlen){
  size_t len = strnlen(str, maxlen);
  char* dup = (char *) malloc(len + 1);
  memcpy(dup, str, len);
  dup[len] = 0;
  return dup;
}
#endif



  /**
   * Copies string from pointer to fixed size C-style array. Src needs to be a valid C-string - eg. ended with '\0'.
   * Throws when source is bigger than destination.
   * @tparam T type of destination array
   * @param dest fixed size destination array
   * @param src pointer to source c-style valid string
   */
    template <typename T>
    void strcpyT(T& dest, const char* src){

      if (src == nullptr)
//            throw EmptySourceStringException(slot_number);
            return;
        const size_t s_dest = sizeof dest;
      LOG(std::string("strcpyT sizes dest src ")
                                     +std::to_string(s_dest)+ " "
                                     +std::to_string(strlen(src))+ " "
          ,nitrokey::log::Loglevel::DEBUG_L2);
        if (strlen(src) > s_dest){
            throw TooLongStringException(strlen(src), s_dest, src);
        }
        strncpy((char*) &dest, src, s_dest);
    }

    template <typename T>
    typename T::CommandPayload get_payload(){
        //Create, initialize and return by value command payload
        typename T::CommandPayload st;
        bzero(&st, sizeof(st));
        return st;
    }


    // package type to auth, auth type [Authorize,UserAuthorize]
    template <typename S, typename A, typename T>
    void NitrokeyManager::authorize_packet(T &package, const char *admin_temporary_password, shared_ptr<Device> device){
      if (!is_authorization_command_supported()){
        LOG("Authorization command not supported, skipping", Loglevel::WARNING);
      }
        auto auth = get_payload<A>();
        strcpyT(auth.temporary_password, admin_temporary_password);
        auth.crc_to_authorize = S::CommandTransaction::getCRC(package);
        A::CommandTransaction::run(device, auth);
    }

    shared_ptr <NitrokeyManager> NitrokeyManager::_instance = nullptr;

    NitrokeyManager::NitrokeyManager() : device(nullptr)
    {
        set_debug(true);
    }
    NitrokeyManager::~NitrokeyManager() {
    }

    bool NitrokeyManager::set_current_device_speed(int retry_delay, int send_receive_delay){
      if (retry_delay < 20 || send_receive_delay < 20){
        LOG("Delay set too low: " + to_string(retry_delay) +" "+ to_string(send_receive_delay), Loglevel::WARNING);
        return false;
      }

      std::lock_guard<std::mutex> lock(mex_dev_com_manager);
      if(device == nullptr) {
        return false;
      }
      device->set_receiving_delay(std::chrono::duration<int, std::milli>(send_receive_delay));
      device->set_retry_delay(std::chrono::duration<int, std::milli>(retry_delay));
      return true;
    }

    bool NitrokeyManager::connect() {
        std::lock_guard<std::mutex> lock(mex_dev_com_manager);
        vector< shared_ptr<Device> > devices = { make_shared<Stick10>(), make_shared<Stick20>() };
        for( auto & d : devices ){
            if (d->connect()){
                device = std::shared_ptr<Device>(d);
            }
        }
        return device != nullptr;
    }


    void NitrokeyManager::set_log_function(std::function<void(std::string)> log_function){
      static nitrokey::log::FunctionalLogHandler handler(log_function);
      nitrokey::log::Log::instance().set_handler(&handler);
    }

    bool NitrokeyManager::set_default_commands_delay(int delay){
      if (delay < 20){
        LOG("Delay set too low: " + to_string(delay), Loglevel::WARNING);
        return false;
      }
      Device::set_default_device_speed(delay);
      return true;
    }

    bool NitrokeyManager::connect(const char *device_model) {
      std::lock_guard<std::mutex> lock(mex_dev_com_manager);
      LOG(__FUNCTION__, nitrokey::log::Loglevel::DEBUG_L2);
      switch (device_model[0]){
            case 'P':
                device = make_shared<Stick10>();
                break;
            case 'S':
                device = make_shared<Stick20>();
                break;
            default:
                throw std::runtime_error("Unknown model");
        }
        return device->connect();
    }

    shared_ptr<NitrokeyManager> NitrokeyManager::instance() {
      static std::mutex mutex;
      std::lock_guard<std::mutex> lock(mutex);
        if (_instance == nullptr){
            _instance = make_shared<NitrokeyManager>();
        }
        return _instance;
    }



    bool NitrokeyManager::disconnect() {
      std::lock_guard<std::mutex> lock(mex_dev_com_manager);
      return _disconnect_no_lock();
    }

  bool NitrokeyManager::_disconnect_no_lock() {
    //do not use directly without locked mutex,
    //used by could_be_enumerated, disconnect
    if (device == nullptr){
      return false;
    }
    const auto res = device->disconnect();
    device = nullptr;
    return res;
  }

  bool NitrokeyManager::is_connected() throw(){
      std::lock_guard<std::mutex> lock(mex_dev_com_manager);
      if(device != nullptr){
        auto connected = device->could_be_enumerated();
        if(connected){
          return true;
        } else {
          _disconnect_no_lock();
          return false;
        }
      }
      return false;
  }

  bool NitrokeyManager::could_current_device_be_enumerated() {
    std::lock_guard<std::mutex> lock(mex_dev_com_manager);
    if (device != nullptr) {
      return device->could_be_enumerated();
    }
    return false;
  }

    void NitrokeyManager::set_loglevel(Loglevel loglevel) {
      Log::instance().set_loglevel(loglevel);
    }

    void NitrokeyManager::set_debug(bool state) {
        if (state){
            Log::instance().set_loglevel(Loglevel::DEBUG);
        } else {
            Log::instance().set_loglevel(Loglevel::ERROR);
        }
    }


    string NitrokeyManager::get_serial_number() {
        if (device == nullptr) { return ""; };
      switch (device->get_device_model()) {
        case DeviceModel::PRO: {
          auto response = GetStatus::CommandTransaction::run(device);
          return nitrokey::misc::toHex(response.data().card_serial_u32);
        }
          break;

        case DeviceModel::STORAGE:
        {
          auto response = stick20::GetDeviceStatus::CommandTransaction::run(device);
          return nitrokey::misc::toHex(response.data().ActiveSmartCardID_u32);
        }
          break;
      }
      return "NA";
    }

    stick10::GetStatus::ResponsePayload NitrokeyManager::get_status(){
      try{
        auto response = GetStatus::CommandTransaction::run(device);
        return response.data();
      }
      catch (DeviceSendingFailure &e){
        disconnect();
        throw;
      }
    }

    string NitrokeyManager::get_status_as_string() {
        auto response = GetStatus::CommandTransaction::run(device);
        return response.data().dissect();
    }

    string getFilledOTPCode(uint32_t code, bool use_8_digits){
      stringstream s;
      s << std::right << std::setw(use_8_digits ? 8 : 6) << std::setfill('0') << code;
      return s.str();
    }

    string NitrokeyManager::get_HOTP_code(uint8_t slot_number, const char *user_temporary_password) {
      if (!is_valid_hotp_slot_number(slot_number)) throw InvalidSlotException(slot_number);

      if (is_authorization_command_supported()){
        auto gh = get_payload<GetHOTP>();
        gh.slot_number = get_internal_slot_number_for_hotp(slot_number);
        if(user_temporary_password != nullptr && strlen(user_temporary_password)!=0){ //FIXME use string instead of strlen
            authorize_packet<GetHOTP, UserAuthorize>(gh, user_temporary_password, device);
        }
        auto resp = GetHOTP::CommandTransaction::run(device, gh);
        return getFilledOTPCode(resp.data().code, resp.data().use_8_digits);
      } else {
        auto gh = get_payload<stick10_08::GetHOTP>();
        gh.slot_number = get_internal_slot_number_for_hotp(slot_number);
        if(user_temporary_password != nullptr && strlen(user_temporary_password)!=0) {
          strcpyT(gh.temporary_user_password, user_temporary_password);
        }
        auto resp = stick10_08::GetHOTP::CommandTransaction::run(device, gh);
        return getFilledOTPCode(resp.data().code, resp.data().use_8_digits);
      }
      return "";
    }

    bool NitrokeyManager::is_valid_hotp_slot_number(uint8_t slot_number) const { return slot_number < 3; }
    bool NitrokeyManager::is_valid_totp_slot_number(uint8_t slot_number) const { return slot_number < 0x10-1; } //15
    uint8_t NitrokeyManager::get_internal_slot_number_for_totp(uint8_t slot_number) const { return (uint8_t) (0x20 + slot_number); }
    uint8_t NitrokeyManager::get_internal_slot_number_for_hotp(uint8_t slot_number) const { return (uint8_t) (0x10 + slot_number); }



    string NitrokeyManager::get_TOTP_code(uint8_t slot_number, uint64_t challenge, uint64_t last_totp_time,
                                          uint8_t last_interval,
                                          const char *user_temporary_password) {
        if(!is_valid_totp_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        slot_number = get_internal_slot_number_for_totp(slot_number);

        if (is_authorization_command_supported()){
          auto gt = get_payload<GetTOTP>();
          gt.slot_number = slot_number;
          gt.challenge = challenge;
          gt.last_interval = last_interval;
          gt.last_totp_time = last_totp_time;

          if(user_temporary_password != nullptr && strlen(user_temporary_password)!=0){ //FIXME use string instead of strlen
              authorize_packet<GetTOTP, UserAuthorize>(gt, user_temporary_password, device);
          }
          auto resp = GetTOTP::CommandTransaction::run(device, gt);
          return getFilledOTPCode(resp.data().code, resp.data().use_8_digits);
        } else {
          auto gt = get_payload<stick10_08::GetTOTP>();
          strcpyT(gt.temporary_user_password, user_temporary_password);
          gt.slot_number = slot_number;
          auto resp = stick10_08::GetTOTP::CommandTransaction::run(device, gt);
          return getFilledOTPCode(resp.data().code, resp.data().use_8_digits);
        }
      return "";
    }

    bool NitrokeyManager::erase_slot(uint8_t slot_number, const char *temporary_password) {
      if (is_authorization_command_supported()){
        auto p = get_payload<EraseSlot>();
        p.slot_number = slot_number;
        authorize_packet<EraseSlot, Authorize>(p, temporary_password, device);
        auto resp = EraseSlot::CommandTransaction::run(device,p);
      } else {
        auto p = get_payload<stick10_08::EraseSlot>();
        p.slot_number = slot_number;
        strcpyT(p.temporary_admin_password, temporary_password);
        auto resp = stick10_08::EraseSlot::CommandTransaction::run(device,p);
      }
        return true;
    }

    bool NitrokeyManager::erase_hotp_slot(uint8_t slot_number, const char *temporary_password) {
        if (!is_valid_hotp_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        slot_number = get_internal_slot_number_for_hotp(slot_number);
        return erase_slot(slot_number, temporary_password);
    }

    bool NitrokeyManager::erase_totp_slot(uint8_t slot_number, const char *temporary_password) {
        if (!is_valid_totp_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        slot_number = get_internal_slot_number_for_totp(slot_number);
        return erase_slot(slot_number, temporary_password);
    }

    template <typename T, typename U>
    void vector_copy_ranged(T& dest, std::vector<U> &vec, size_t begin, size_t elements_to_copy){
        const size_t d_size = sizeof(dest);
      if(d_size < elements_to_copy){
            throw TargetBufferSmallerThanSource(elements_to_copy, d_size);
        }
        std::fill(dest, dest+d_size, 0);
        std::copy(vec.begin() + begin, vec.begin() +begin + elements_to_copy, dest);
    }

    template <typename T, typename U>
    void vector_copy(T& dest, std::vector<U> &vec){
        const size_t d_size = sizeof(dest);
        if(d_size < vec.size()){
            throw TargetBufferSmallerThanSource(vec.size(), d_size);
        }
        std::fill(dest, dest+d_size, 0);
        std::copy(vec.begin(), vec.end(), dest);
    }

    bool NitrokeyManager::write_HOTP_slot(uint8_t slot_number, const char *slot_name, const char *secret, uint64_t hotp_counter,
                                          bool use_8_digits, bool use_enter, bool use_tokenID, const char *token_ID,
                                          const char *temporary_password) {
        if (!is_valid_hotp_slot_number(slot_number)) throw InvalidSlotException(slot_number);

      int internal_slot_number = get_internal_slot_number_for_hotp(slot_number);
      if (is_authorization_command_supported()){
        write_HOTP_slot_authorize(internal_slot_number, slot_name, secret, hotp_counter, use_8_digits, use_enter, use_tokenID,
                    token_ID, temporary_password);
      } else {
        write_OTP_slot_no_authorize(internal_slot_number, slot_name, secret, hotp_counter, use_8_digits, use_enter, use_tokenID,
                                    token_ID, temporary_password);
      }
      return true;
    }

    void NitrokeyManager::write_HOTP_slot_authorize(uint8_t slot_number, const char *slot_name, const char *secret,
                                                    uint64_t hotp_counter, bool use_8_digits, bool use_enter,
                                                    bool use_tokenID, const char *token_ID, const char *temporary_password) {
      auto payload = get_payload<WriteToHOTPSlot>();
      payload.slot_number = slot_number;
      auto secret_bin = misc::hex_string_to_byte(secret);
      vector_copy(payload.slot_secret, secret_bin);
      strcpyT(payload.slot_name, slot_name);
      strcpyT(payload.slot_token_id, token_ID);
      switch (device->get_device_model() ){
        case DeviceModel::PRO: {
          payload.slot_counter = hotp_counter;
          break;
        }
        case DeviceModel::STORAGE: {
          string counter = to_string(hotp_counter);
          strcpyT(payload.slot_counter_s, counter.c_str());
          break;
        }
        default:
          LOG(string(__FILE__) + to_string(__LINE__) +
                          string(__FUNCTION__) + string(" Unhandled device model for HOTP")
              , Loglevel::DEBUG);
          break;
      }
      payload.use_8_digits = use_8_digits;
      payload.use_enter = use_enter;
      payload.use_tokenID = use_tokenID;

      authorize_packet<WriteToHOTPSlot, Authorize>(payload, temporary_password, device);

      auto resp = WriteToHOTPSlot::CommandTransaction::run(device, payload);
    }

    bool NitrokeyManager::write_TOTP_slot(uint8_t slot_number, const char *slot_name, const char *secret, uint16_t time_window,
                                              bool use_8_digits, bool use_enter, bool use_tokenID, const char *token_ID,
                                              const char *temporary_password) {
        if (!is_valid_totp_slot_number(slot_number)) throw InvalidSlotException(slot_number);
       int internal_slot_number = get_internal_slot_number_for_totp(slot_number);

      if (is_authorization_command_supported()){
      write_TOTP_slot_authorize(internal_slot_number, slot_name, secret, time_window, use_8_digits, use_enter, use_tokenID,
                                token_ID, temporary_password);
      } else {
        write_OTP_slot_no_authorize(internal_slot_number, slot_name, secret, time_window, use_8_digits, use_enter, use_tokenID,
                                    token_ID, temporary_password);
      }

      return true;
    }

    void NitrokeyManager::write_OTP_slot_no_authorize(uint8_t internal_slot_number, const char *slot_name,
                                                      const char *secret,
                                                      uint64_t counter_or_interval, bool use_8_digits, bool use_enter,
                                                      bool use_tokenID, const char *token_ID,
                                                      const char *temporary_password) const {

      auto payload2 = get_payload<stick10_08::SendOTPData>();
      strcpyT(payload2.temporary_admin_password, temporary_password);
      strcpyT(payload2.data, slot_name);
      payload2.setTypeName();
      stick10_08::SendOTPData::CommandTransaction::run(device, payload2);

      payload2.setTypeSecret();
      payload2.id = 0;
      auto secret_bin = misc::hex_string_to_byte(secret);
      auto remaining_secret_length = secret_bin.size();
      const auto maximum_OTP_secret_size = 40;
      if(remaining_secret_length > maximum_OTP_secret_size){
        throw TargetBufferSmallerThanSource(remaining_secret_length, maximum_OTP_secret_size);
      }

      while (remaining_secret_length>0){
        const auto bytesToCopy = std::min(sizeof(payload2.data), remaining_secret_length);
        const auto start = secret_bin.size() - remaining_secret_length;
        memset(payload2.data, 0, sizeof(payload2.data));
        vector_copy_ranged(payload2.data, secret_bin, start, bytesToCopy);
        stick10_08::SendOTPData::CommandTransaction::run(device, payload2);
        remaining_secret_length -= bytesToCopy;
        payload2.id++;
      }

      auto payload = get_payload<stick10_08::WriteToOTPSlot>();
      strcpyT(payload.temporary_admin_password, temporary_password);
      strcpyT(payload.slot_token_id, token_ID);
      payload.use_8_digits = use_8_digits;
      payload.use_enter = use_enter;
      payload.use_tokenID = use_tokenID;
      payload.slot_counter_or_interval = counter_or_interval;
      payload.slot_number = internal_slot_number;
      stick10_08::WriteToOTPSlot::CommandTransaction::run(device, payload);
    }

    void NitrokeyManager::write_TOTP_slot_authorize(uint8_t slot_number, const char *slot_name, const char *secret,
                                                    uint16_t time_window, bool use_8_digits, bool use_enter,
                                                    bool use_tokenID, const char *token_ID, const char *temporary_password) {
      auto payload = get_payload<WriteToTOTPSlot>();
      payload.slot_number = slot_number;
      auto secret_bin = misc::hex_string_to_byte(secret);
      vector_copy(payload.slot_secret, secret_bin);
      strcpyT(payload.slot_name, slot_name);
      strcpyT(payload.slot_token_id, token_ID);
      payload.slot_interval = time_window; //FIXME naming
      payload.use_8_digits = use_8_digits;
      payload.use_enter = use_enter;
      payload.use_tokenID = use_tokenID;

      authorize_packet<WriteToTOTPSlot, Authorize>(payload, temporary_password, device);

      auto resp = WriteToTOTPSlot::CommandTransaction::run(device, payload);
    }

    const char * NitrokeyManager::get_totp_slot_name(uint8_t slot_number) {
        if (!is_valid_totp_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        slot_number = get_internal_slot_number_for_totp(slot_number);
        return get_slot_name(slot_number);
    }
    const char * NitrokeyManager::get_hotp_slot_name(uint8_t slot_number) {
        if (!is_valid_hotp_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        slot_number = get_internal_slot_number_for_hotp(slot_number);
        return get_slot_name(slot_number);
    }

  static const int max_string_field_length = 100;

  const char * NitrokeyManager::get_slot_name(uint8_t slot_number)  {
        auto payload = get_payload<GetSlotName>();
        payload.slot_number = slot_number;
        auto resp = GetSlotName::CommandTransaction::run(device, payload);
        return strndup((const char *) resp.data().slot_name, max_string_field_length);
    }

    bool NitrokeyManager::first_authenticate(const char *pin, const char *temporary_password) {
        auto authreq = get_payload<FirstAuthenticate>();
        strcpyT(authreq.card_password, pin);
        strcpyT(authreq.temporary_password, temporary_password);
        FirstAuthenticate::CommandTransaction::run(device, authreq);
        return true;
    }

    bool NitrokeyManager::set_time(uint64_t time) {
        auto p = get_payload<SetTime>();
        p.reset = 1;
        p.time = time;
        SetTime::CommandTransaction::run(device, p);
        return false;
    }

    bool NitrokeyManager::get_time(uint64_t time) {
        auto p = get_payload<SetTime>();
        p.reset = 0;
        p.time = time;
        SetTime::CommandTransaction::run(device, p);
        return true;
    }

    void NitrokeyManager::change_user_PIN(const char *current_PIN, const char *new_PIN) {
        change_PIN_general<ChangeUserPin, PasswordKind::User>(current_PIN, new_PIN);
    }

    void NitrokeyManager::change_admin_PIN(const char *current_PIN, const char *new_PIN) {
        change_PIN_general<ChangeAdminPin, PasswordKind::Admin>(current_PIN, new_PIN);
    }

    template <typename ProCommand, PasswordKind StoKind>
    void NitrokeyManager::change_PIN_general(const char *current_PIN, const char *new_PIN) {
        switch (device->get_device_model()){
            case DeviceModel::PRO:
            {
                auto p = get_payload<ProCommand>();
                strcpyT(p.old_pin, current_PIN);
                strcpyT(p.new_pin, new_PIN);
                ProCommand::CommandTransaction::run(device, p);
            }
                break;
            //in Storage change admin/user pin is divided to two commands with 20 chars field len
            case DeviceModel::STORAGE:
            {
                auto p = get_payload<ChangeAdminUserPin20Current>();
                strcpyT(p.password, current_PIN);
                p.set_kind(StoKind);
                auto p2 = get_payload<ChangeAdminUserPin20New>();
                strcpyT(p2.password, new_PIN);
                p2.set_kind(StoKind);
                ChangeAdminUserPin20Current::CommandTransaction::run(device, p);
                ChangeAdminUserPin20New::CommandTransaction::run(device, p2);
            }
                break;
        }

    }

    void NitrokeyManager::enable_password_safe(const char *user_pin) {
        //The following command will cancel enabling PWS if it is not supported
        auto a = get_payload<IsAESSupported>();
        strcpyT(a.user_password, user_pin);
        IsAESSupported::CommandTransaction::run(device, a);

        auto p = get_payload<EnablePasswordSafe>();
        strcpyT(p.user_password, user_pin);
        EnablePasswordSafe::CommandTransaction::run(device, p);
    }

    vector <uint8_t> NitrokeyManager::get_password_safe_slot_status() {
        auto responsePayload = GetPasswordSafeSlotStatus::CommandTransaction::run(device);
        vector<uint8_t> v = vector<uint8_t>(responsePayload.data().password_safe_status,
                                            responsePayload.data().password_safe_status
                                            + sizeof(responsePayload.data().password_safe_status));
        return v;
    }

    uint8_t NitrokeyManager::get_user_retry_count() {
        if(device->get_device_model() == DeviceModel::STORAGE){
          stick20::GetDeviceStatus::CommandTransaction::run(device);
        }
        auto response = GetUserPasswordRetryCount::CommandTransaction::run(device);
        return response.data().password_retry_count;
    }

    uint8_t NitrokeyManager::get_admin_retry_count() {
        if(device->get_device_model() == DeviceModel::STORAGE){
          stick20::GetDeviceStatus::CommandTransaction::run(device);
        }
        auto response = GetPasswordRetryCount::CommandTransaction::run(device);
        return response.data().password_retry_count;
    }

    void NitrokeyManager::lock_device() {
        LockDevice::CommandTransaction::run(device);
    }

    const char *NitrokeyManager::get_password_safe_slot_name(uint8_t slot_number) {
        if (!is_valid_password_safe_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        auto p = get_payload<GetPasswordSafeSlotName>();
        p.slot_number = slot_number;
        auto response = GetPasswordSafeSlotName::CommandTransaction::run(device, p);
        return strndup((const char *) response.data().slot_name, max_string_field_length);
    }

    bool NitrokeyManager::is_valid_password_safe_slot_number(uint8_t slot_number) const { return slot_number < 16; }

    const char *NitrokeyManager::get_password_safe_slot_login(uint8_t slot_number) {
        if (!is_valid_password_safe_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        auto p = get_payload<GetPasswordSafeSlotLogin>();
        p.slot_number = slot_number;
        auto response = GetPasswordSafeSlotLogin::CommandTransaction::run(device, p);
        return strndup((const char *) response.data().slot_login, max_string_field_length);
    }

    const char *NitrokeyManager::get_password_safe_slot_password(uint8_t slot_number) {
        if (!is_valid_password_safe_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        auto p = get_payload<GetPasswordSafeSlotPassword>();
        p.slot_number = slot_number;
        auto response = GetPasswordSafeSlotPassword::CommandTransaction::run(device, p);
        return strndup((const char *) response.data().slot_password, max_string_field_length); //FIXME use secure way
    }

    void NitrokeyManager::write_password_safe_slot(uint8_t slot_number, const char *slot_name, const char *slot_login,
                                                       const char *slot_password) {
        if (!is_valid_password_safe_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        auto p = get_payload<SetPasswordSafeSlotData>();
        p.slot_number = slot_number;
        strcpyT(p.slot_name, slot_name);
        strcpyT(p.slot_password, slot_password);
        SetPasswordSafeSlotData::CommandTransaction::run(device, p);

        auto p2 = get_payload<SetPasswordSafeSlotData2>();
        p2.slot_number = slot_number;
        strcpyT(p2.slot_login_name, slot_login);
        SetPasswordSafeSlotData2::CommandTransaction::run(device, p2);
    }

    void NitrokeyManager::erase_password_safe_slot(uint8_t slot_number) {
        if (!is_valid_password_safe_slot_number(slot_number)) throw InvalidSlotException(slot_number);
        auto p = get_payload<ErasePasswordSafeSlot>();
        p.slot_number = slot_number;
        ErasePasswordSafeSlot::CommandTransaction::run(device, p);
    }

    void NitrokeyManager::user_authenticate(const char *user_password, const char *temporary_password) {
        auto p = get_payload<UserAuthenticate>();
        strcpyT(p.card_password, user_password);
        strcpyT(p.temporary_password, temporary_password);
        UserAuthenticate::CommandTransaction::run(device, p);
    }

    void NitrokeyManager::build_aes_key(const char *admin_password) {
        switch (device->get_device_model()) {
            case DeviceModel::PRO: {
                auto p = get_payload<BuildAESKey>();
                strcpyT(p.admin_password, admin_password);
                BuildAESKey::CommandTransaction::run(device, p);
                break;
            }
            case DeviceModel::STORAGE : {
                auto p = get_payload<stick20::CreateNewKeys>();
                strcpyT(p.password, admin_password);
                p.set_defaults();
                stick20::CreateNewKeys::CommandTransaction::run(device, p);
                break;
            }
        }
    }

    void NitrokeyManager::factory_reset(const char *admin_password) {
        auto p = get_payload<FactoryReset>();
        strcpyT(p.admin_password, admin_password);
        FactoryReset::CommandTransaction::run(device, p);
    }

    void NitrokeyManager::unlock_user_password(const char *admin_password, const char *new_user_password) {
      switch (device->get_device_model()){
        case DeviceModel::PRO: {
          auto p = get_payload<stick10::UnlockUserPassword>();
          strcpyT(p.admin_password, admin_password);
          strcpyT(p.user_new_password, new_user_password);
          stick10::UnlockUserPassword::CommandTransaction::run(device, p);
          break;
        }
        case DeviceModel::STORAGE : {
          auto p2 = get_payload<ChangeAdminUserPin20Current>();
          p2.set_defaults();
          strcpyT(p2.password, admin_password);
          ChangeAdminUserPin20Current::CommandTransaction::run(device, p2);
          auto p3 = get_payload<stick20::UnlockUserPin>();
          p3.set_defaults();
          strcpyT(p3.password, new_user_password);
          stick20::UnlockUserPin::CommandTransaction::run(device, p3);
          break;
        }
      }
    }


    void NitrokeyManager::write_config(uint8_t numlock, uint8_t capslock, uint8_t scrolllock, bool enable_user_password,
                                       bool delete_user_password, const char *admin_temporary_password) {
        auto p = get_payload<stick10_08::WriteGeneralConfig>();
        p.numlock = (uint8_t) numlock;
        p.capslock = (uint8_t) capslock;
        p.scrolllock = (uint8_t) scrolllock;
        p.enable_user_password = (uint8_t) enable_user_password;
        p.delete_user_password = (uint8_t) delete_user_password;
        if (is_authorization_command_supported()){
          authorize_packet<stick10_08::WriteGeneralConfig, Authorize>(p, admin_temporary_password, device);
        } else {
          strcpyT(p.temporary_admin_password, admin_temporary_password);
        }
        stick10_08::WriteGeneralConfig::CommandTransaction::run(device, p);
    }

    vector<uint8_t> NitrokeyManager::read_config() {
        auto responsePayload = GetStatus::CommandTransaction::run(device);
        vector<uint8_t> v = vector<uint8_t>(responsePayload.data().general_config,
                                            responsePayload.data().general_config+sizeof(responsePayload.data().general_config));
        return v;
    }

    bool NitrokeyManager::is_authorization_command_supported(){
      //authorization command is supported for versions equal or below:
        auto m = std::unordered_map<DeviceModel , int, EnumClassHash>({
                                               {DeviceModel::PRO, 7},
                                               {DeviceModel::STORAGE, 999},
         });
        return get_minor_firmware_version() <= m[device->get_device_model()];
    }

    bool NitrokeyManager::is_320_OTP_secret_supported(){
      //authorization command is supported for versions equal or below:
        auto m = std::unordered_map<DeviceModel , int, EnumClassHash>({
                                               {DeviceModel::PRO, 8},
                                               {DeviceModel::STORAGE, 999},
         });
        return get_minor_firmware_version() >= m[device->get_device_model()];
    }

    DeviceModel NitrokeyManager::get_connected_device_model() const{
      //FIXME throw if no device is connected or return unknown/unconnected value
        if (device == nullptr){
            throw std::runtime_error("device not connected");
        }
      return device->get_device_model();
    }

    int NitrokeyManager::get_minor_firmware_version(){
      switch(device->get_device_model()){
        case DeviceModel::PRO:{
          auto status_p = GetStatus::CommandTransaction::run(device);
          return status_p.data().firmware_version; //7 or 8
        }
        case DeviceModel::STORAGE:{
          auto status = stick20::GetDeviceStatus::CommandTransaction::run(device);
          return status.data().versionInfo.minor;
        }
      }
      return 0;
    }

    bool NitrokeyManager::is_AES_supported(const char *user_password) {
        auto a = get_payload<IsAESSupported>();
        strcpyT(a.user_password, user_password);
        IsAESSupported::CommandTransaction::run(device, a);
        return true;
    }

    //storage commands

    void NitrokeyManager::send_startup(uint64_t seconds_from_epoch){
      auto p = get_payload<stick20::SendStartup>();
//      p.set_defaults(); //set current time
      p.localtime = seconds_from_epoch;
      stick20::SendStartup::CommandTransaction::run(device, p);
    }

    void NitrokeyManager::unlock_encrypted_volume(const char* user_pin){
      misc::execute_password_command<stick20::EnableEncryptedPartition>(device, user_pin);
    }

    void NitrokeyManager::unlock_hidden_volume(const char* hidden_volume_password) {
      misc::execute_password_command<stick20::EnableHiddenEncryptedPartition>(device, hidden_volume_password);
    }

    //TODO check is encrypted volume unlocked before execution
    //if not return library exception
    void NitrokeyManager::create_hidden_volume(uint8_t slot_nr, uint8_t start_percent, uint8_t end_percent,
                                               const char *hidden_volume_password) {
      auto p = get_payload<stick20::SetupHiddenVolume>();
      p.SlotNr_u8 = slot_nr;
      p.StartBlockPercent_u8 = start_percent;
      p.EndBlockPercent_u8 = end_percent;
      strcpyT(p.HiddenVolumePassword_au8, hidden_volume_password);
      stick20::SetupHiddenVolume::CommandTransaction::run(device, p);
    }

    void NitrokeyManager::set_unencrypted_read_only(const char* user_pin) {
      misc::execute_password_command<stick20::SendSetReadonlyToUncryptedVolume>(device, user_pin);
    }

    void NitrokeyManager::set_unencrypted_read_write(const char* user_pin) {
      misc::execute_password_command<stick20::SendSetReadwriteToUncryptedVolume>(device, user_pin);
    }

    void NitrokeyManager::export_firmware(const char* admin_pin) {
      misc::execute_password_command<stick20::ExportFirmware>(device, admin_pin);
    }

    void NitrokeyManager::enable_firmware_update(const char* firmware_pin) {
      misc::execute_password_command<stick20::EnableFirmwareUpdate>(device, firmware_pin);
    }

    void NitrokeyManager::clear_new_sd_card_warning(const char* admin_pin) {
      misc::execute_password_command<stick20::SendClearNewSdCardFound>(device, admin_pin);
    }

    void NitrokeyManager::fill_SD_card_with_random_data(const char* admin_pin) {
      auto p = get_payload<stick20::FillSDCardWithRandomChars>();
      p.set_defaults();
      strcpyT(p.admin_pin, admin_pin);
      stick20::FillSDCardWithRandomChars::CommandTransaction::run(device, p);
    }

    void NitrokeyManager::change_update_password(const char* current_update_password, const char* new_update_password) {
      auto p = get_payload<stick20::ChangeUpdatePassword>();
      strcpyT(p.current_update_password, current_update_password);
      strcpyT(p.new_update_password, new_update_password);
      stick20::ChangeUpdatePassword::CommandTransaction::run(device, p);
    }

    const char * NitrokeyManager::get_status_storage_as_string(){
      auto p = stick20::GetDeviceStatus::CommandTransaction::run(device);
      return strndup(p.data().dissect().c_str(), max_string_field_length);
    }

    stick20::DeviceConfigurationResponsePacket::ResponsePayload NitrokeyManager::get_status_storage(){
      auto p = stick20::GetDeviceStatus::CommandTransaction::run(device);
      return p.data();
    }

    const char * NitrokeyManager::get_SD_usage_data_as_string(){
      auto p = stick20::GetSDCardOccupancy::CommandTransaction::run(device);
      return strndup(p.data().dissect().c_str(), max_string_field_length);
    }

    std::pair<uint8_t,uint8_t> NitrokeyManager::get_SD_usage_data(){
      auto p = stick20::GetSDCardOccupancy::CommandTransaction::run(device);
      return std::make_pair(p.data().WriteLevelMin, p.data().WriteLevelMax);
    }

    int NitrokeyManager::get_progress_bar_value(){
      try{
        stick20::GetDeviceStatus::CommandTransaction::run(device);
        return -1;
      }
      catch (LongOperationInProgressException &e){
        return e.progress_bar_value;
      }
    }

  string NitrokeyManager::get_TOTP_code(uint8_t slot_number, const char *user_temporary_password) {
    return get_TOTP_code(slot_number, 0, 0, 0, user_temporary_password);
  }

  stick10::ReadSlot::ResponsePayload NitrokeyManager::get_OTP_slot_data(const uint8_t slot_number) {
    auto p = get_payload<stick10::ReadSlot>();
    p.slot_number = slot_number;
    auto data = stick10::ReadSlot::CommandTransaction::run(device, p);
    return data.data();
  }

  stick10::ReadSlot::ResponsePayload NitrokeyManager::get_TOTP_slot_data(const uint8_t slot_number) {
    return get_OTP_slot_data(get_internal_slot_number_for_totp(slot_number));
  }

  stick10::ReadSlot::ResponsePayload NitrokeyManager::get_HOTP_slot_data(const uint8_t slot_number) {
    auto slot_data = get_OTP_slot_data(get_internal_slot_number_for_hotp(slot_number));
    if (device->get_device_model() == DeviceModel::STORAGE){
      //convert counter from string to ull
      auto counter_s = std::string(slot_data.slot_counter_s, slot_data.slot_counter_s+sizeof(slot_data.slot_counter_s));
      slot_data.slot_counter = std::stoull(counter_s);
    }
    return slot_data;
  }

  void NitrokeyManager::lock_encrypted_volume() {
    misc::execute_password_command<stick20::DisableEncryptedPartition>(device, "");
  }

  void NitrokeyManager::lock_hidden_volume() {
    misc::execute_password_command<stick20::DisableHiddenEncryptedPartition>(device, "");
  }

  uint8_t NitrokeyManager::get_SD_card_size() {
    auto data = stick20::ProductionTest::CommandTransaction::run(device);
    return data.data().SD_Card_Size_u8;
  }


}
