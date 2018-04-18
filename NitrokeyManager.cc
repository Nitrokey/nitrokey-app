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

#include <cstring>
#include <iostream>
#include "libnitrokey/NitrokeyManager.h"
#include "libnitrokey/LibraryException.h"
#include <algorithm>
#include <unordered_map>
#include <stick20_commands.h>
#include "libnitrokey/misc.h"
#include <mutex>
#include "libnitrokey/cxx_semantics.h"
#include <functional>
#include <stick10_commands.h>

std::mutex nitrokey::proto::send_receive_mtx;

namespace nitrokey{

    std::mutex mex_dev_com_manager;

#ifndef strndup
#ifdef _WIN32
#pragma message "Using own strndup"
char * strndup(const char* str, size_t maxlen){
  size_t len = strnlen(str, maxlen);
  char* dup = (char *) malloc(len + 1);
  memcpy(dup, str, len);
  dup[len] = 0;
  return dup;
}
#endif
#endif

using nitrokey::misc::strcpyT;

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
        set_debug(false);
    }
    NitrokeyManager::~NitrokeyManager() {
        std::lock_guard<std::mutex> lock(mex_dev_com_manager);

        for (auto d : connected_devices){
            if (d.second == nullptr) continue;
            d.second->disconnect();
            connected_devices[d.first] = nullptr;
        }
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

    std::vector<std::string> NitrokeyManager::list_devices(){
        std::lock_guard<std::mutex> lock(mex_dev_com_manager);

        auto p = make_shared<Stick20>();
        return p->enumerate(); // make static
    }

    std::vector<std::string> NitrokeyManager::list_devices_by_cpuID(){
        using misc::toHex;
        //disconnect default device
        disconnect();

        std::lock_guard<std::mutex> lock(mex_dev_com_manager);
        LOGD1("Disconnecting registered devices");
        for (auto & kv : connected_devices_byID){
            if (kv.second != nullptr)
                kv.second->disconnect();
        }
        connected_devices_byID.clear();

        LOGD1("Enumerating devices");
        std::vector<std::string> res;
        auto d = make_shared<Stick20>();
        const auto v = d->enumerate();
        LOGD1("Discovering IDs");
        for (auto & p: v){
            d = make_shared<Stick20>();
            LOGD1( std::string("Found: ") + p );
            d->set_path(p);
            try{
                if (d->connect()){
                    device = d;
                    std::string id;
                    try {
                        const auto status = get_status_storage();
                        const auto sc_id = toHex(status.ActiveSmartCardID_u32);
                        const auto sd_id = toHex(status.ActiveSD_CardID_u32);
                        id += sc_id + ":" + sd_id;
                        id += "_p_" + p;
                    }
                    catch (const LongOperationInProgressException &e) {
                        LOGD1(std::string("Long operation in progress, setting ID to: ") + p);
                        id = p;
                    }

                    connected_devices_byID[id] = d;
                    res.push_back(id);
                    LOGD1( std::string("Found: ") + p + " => " + id);
                } else{
                    LOGD1( std::string("Could not connect to: ") + p);
                }
            }
            catch (const DeviceCommunicationException &e){
                LOGD1( std::string("Exception encountered: ") + p);
            }
        }
        return res;
    }

    bool NitrokeyManager::connect_with_ID(const std::string id) {
        std::lock_guard<std::mutex> lock(mex_dev_com_manager);

        auto position = connected_devices_byID.find(id);
        if (position == connected_devices_byID.end()) {
            LOGD1(std::string("Could not find device ")+id + ". Refresh devices list with list_devices_by_cpuID().");
            return false;
        }

        auto d = connected_devices_byID[id];
        device = d;
        current_device_id = id;

        //validate connection
        try{
            get_status();
        }
        catch (const LongOperationInProgressException &){
            //ignore
        }
        catch (const DeviceCommunicationException &){
            d->disconnect();
            current_device_id = "";
            connected_devices_byID[id] = nullptr;
            connected_devices_byID.erase(position);
            return false;
        }
        nitrokey::log::Log::setPrefix(id);
        LOGD1("Device successfully changed");
        return true;
    }

        /**
         * Connects device to path.
         * Assumes devices are not being disconnected and caches connections (param cache_connections).
         * @param path os-dependent device path
         * @return false, when could not connect, true otherwise
         */
    bool NitrokeyManager::connect_with_path(std::string path) {
        const bool cache_connections = false;

        std::lock_guard<std::mutex> lock(mex_dev_com_manager);

        if (cache_connections){
            if(connected_devices.find(path) != connected_devices.end()
                    && connected_devices[path] != nullptr) {
                device = connected_devices[path];
                return true;
            }
        }

        auto p = make_shared<Stick20>();
        p->set_path(path);

        if(!p->connect()) return false;

        if(cache_connections){
            connected_devices [path] = p;
        }

        device = p; //previous device will be disconnected automatically
        current_device_id = path;
        nitrokey::log::Log::setPrefix(path);
        LOGD1("Device successfully changed");
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

    bool NitrokeyManager::connect(device::DeviceModel device_model) {
        const char *model_string;
        switch (device_model) {
            case device::DeviceModel::PRO:
                model_string = "P";
                break;
            case device::DeviceModel::STORAGE:
                model_string = "S";
                break;
            default:
                throw std::runtime_error("Unknown model");
        }
        return connect(model_string);
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

    void NitrokeyManager::set_loglevel(int loglevel) {
      loglevel = max(loglevel, static_cast<int>(Loglevel::ERROR));
      loglevel = min(loglevel, static_cast<int>(Loglevel::DEBUG_L2));
      Log::instance().set_loglevel(static_cast<Loglevel>(loglevel));
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
//        disconnect();
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

  static const int max_string_field_length = 2*1024; //storage's status string is ~1k

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
        p.numlock = numlock;
        p.capslock = capslock;
        p.scrolllock = scrolllock;
        p.enable_user_password = static_cast<uint8_t>(enable_user_password ? 1 : 0);
        p.delete_user_password = static_cast<uint8_t>(delete_user_password ? 1 : 0);
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
        if (device == nullptr){
            throw DeviceNotConnected("device not connected");
        }
      return device->get_device_model();
    }

    bool NitrokeyManager::is_smartcard_in_use(){
      try{
        stick20::CheckSmartcardUsage::CommandTransaction::run(device);
      }
      catch(const CommandFailedException & e){
        return e.reason_smartcard_busy();
      }
      return false;
    }

    int NitrokeyManager::get_minor_firmware_version(){
      switch(device->get_device_model()){
        case DeviceModel::PRO:{
          auto status_p = GetStatus::CommandTransaction::run(device);
          return status_p.data().firmware_version_st.minor; //7 or 8
        }
        case DeviceModel::STORAGE:{
          auto status = stick20::GetDeviceStatus::CommandTransaction::run(device);
          auto test_firmware = status.data().versionInfo.build_iteration != 0;
          if (test_firmware)
            LOG("Development firmware detected. Increasing minor version number.", nitrokey::log::Loglevel::WARNING);
          return status.data().versionInfo.minor + (test_firmware? 1 : 0);
        }
      }
      return 0;
    }
    int NitrokeyManager::get_major_firmware_version(){
      switch(device->get_device_model()){
        case DeviceModel::PRO:{
          auto status_p = GetStatus::CommandTransaction::run(device);
          return status_p.data().firmware_version_st.major; //0
        }
        case DeviceModel::STORAGE:{
          auto status = stick20::GetDeviceStatus::CommandTransaction::run(device);
          return status.data().versionInfo.major;
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

    void NitrokeyManager::set_encrypted_volume_read_only(const char* admin_pin) {
        misc::execute_password_command<stick20::SetEncryptedVolumeReadOnly>(device, admin_pin);
    }

    void NitrokeyManager::set_encrypted_volume_read_write(const char* admin_pin) {
        misc::execute_password_command<stick20::SetEncryptedVolumeReadWrite>(device, admin_pin);
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

    void NitrokeyManager::set_unencrypted_read_only_admin(const char* admin_pin) {
      //from v0.49, v0.52+ it needs Admin PIN
      if (set_unencrypted_volume_rorw_pin_type_user()){
        LOG("set_unencrypted_read_only_admin is not supported for this version of Storage device. "
                "Please update firmware to v0.52+. Doing nothing.", nitrokey::log::Loglevel::WARNING);
        return;
      }
      misc::execute_password_command<stick20::SetUnencryptedVolumeReadOnlyAdmin>(device, admin_pin);
    }

    void NitrokeyManager::set_unencrypted_read_only(const char *user_pin) {
        //until v0.48 (incl. v0.50 and v0.51) User PIN was sufficient
        LOG("set_unencrypted_read_only is deprecated. Use set_unencrypted_read_only_admin instead.",
            nitrokey::log::Loglevel::WARNING);
      if (!set_unencrypted_volume_rorw_pin_type_user()){
        LOG("set_unencrypted_read_only is not supported for this version of Storage device. Doing nothing.",
            nitrokey::log::Loglevel::WARNING);
        return;
      }
      misc::execute_password_command<stick20::SendSetReadonlyToUncryptedVolume>(device, user_pin);
    }

    void NitrokeyManager::set_unencrypted_read_write_admin(const char* admin_pin) {
      //from v0.49, v0.52+ it needs Admin PIN
      if (set_unencrypted_volume_rorw_pin_type_user()){
        LOG("set_unencrypted_read_write_admin is not supported for this version of Storage device. "
                "Please update firmware to v0.52+. Doing nothing.", nitrokey::log::Loglevel::WARNING);
        return;
      }
      misc::execute_password_command<stick20::SetUnencryptedVolumeReadWriteAdmin>(device, admin_pin);
    }

    void NitrokeyManager::set_unencrypted_read_write(const char *user_pin) {
        //until v0.48 (incl. v0.50 and v0.51) User PIN was sufficient
      LOG("set_unencrypted_read_write is deprecated. Use set_unencrypted_read_write_admin instead.",
          nitrokey::log::Loglevel::WARNING);
      if (!set_unencrypted_volume_rorw_pin_type_user()){
        LOG("set_unencrypted_read_write is not supported for this version of Storage device. Doing nothing.",
            nitrokey::log::Loglevel::WARNING);
        return;
      }
      misc::execute_password_command<stick20::SendSetReadwriteToUncryptedVolume>(device, user_pin);
    }

    bool NitrokeyManager::set_unencrypted_volume_rorw_pin_type_user(){
      auto minor_firmware_version = get_minor_firmware_version();
      return minor_firmware_version <= 48 || minor_firmware_version == 50 || minor_firmware_version == 51;
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

    const string NitrokeyManager::get_current_device_id() const {
        return current_device_id;
    }


}
