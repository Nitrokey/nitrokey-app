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

#include "NK_C_API.h"
#include <iostream>
#include "libnitrokey/NitrokeyManager.h"
#include <cstring>
#include "libnitrokey/LibraryException.h"
#include "libnitrokey/cxx_semantics.h"

#ifdef _MSC_VER
#ifdef _WIN32
#pragma message "Using own strndup"
char * strndup(const char* str, size_t maxlen) {
	size_t len = strnlen(str, maxlen);
	char* dup = (char *)malloc(len + 1);
	memcpy(dup, str, len);
	dup[len] = 0;
	return dup;
}
#endif
#endif

using namespace nitrokey;

static uint8_t NK_last_command_status = 0;
static const int max_string_field_length = 100;

template <typename T>
T* duplicate_vector_and_clear(std::vector<T> &v){
    auto d = new T[v.size()];
    std::copy(v.begin(), v.end(), d);
    std::fill(v.begin(), v.end(), 0);
    return d;
}

template <typename T>
uint8_t * get_with_array_result(T func){
    NK_last_command_status = 0;
    try {
        return func();
    }
    catch (CommandFailedException & commandFailedException){
        NK_last_command_status = commandFailedException.last_command_status;
    }
    catch (LibraryException & libraryException){
        NK_last_command_status = libraryException.exception_id();
    }
    catch (const DeviceCommunicationException &deviceException){
      NK_last_command_status = 256-deviceException.getType();
    }
    return nullptr;
}

template <typename T>
const char* get_with_string_result(T func){
    NK_last_command_status = 0;
    try {
        return func();
    }
    catch (CommandFailedException & commandFailedException){
        NK_last_command_status = commandFailedException.last_command_status;
    }
    catch (LibraryException & libraryException){
        NK_last_command_status = libraryException.exception_id();
    }
    catch (const DeviceCommunicationException &deviceException){
      NK_last_command_status = 256-deviceException.getType();
    }
    return "";
}

template <typename T>
auto get_with_result(T func){
    NK_last_command_status = 0;
    try {
        return func();
    }
    catch (CommandFailedException & commandFailedException){
        NK_last_command_status = commandFailedException.last_command_status;
    }
    catch (LibraryException & libraryException){
        NK_last_command_status = libraryException.exception_id();
    }
    catch (const DeviceCommunicationException &deviceException){
      NK_last_command_status = 256-deviceException.getType();
    }
    return static_cast<decltype(func())>(0);
}

template <typename T>
uint8_t get_without_result(T func){
    NK_last_command_status = 0;
    try {
        func();
        return 0;
    }
    catch (CommandFailedException & commandFailedException){
        NK_last_command_status = commandFailedException.last_command_status;
    }
    catch (LibraryException & libraryException){
        NK_last_command_status = libraryException.exception_id();
    }
    catch (const InvalidCRCReceived &invalidCRCException){
      ;
    }
    catch (const DeviceCommunicationException &deviceException){
        NK_last_command_status = 256-deviceException.getType();
    }
    return NK_last_command_status;
}


#ifdef __cplusplus
extern "C" {
#endif

	NK_C_API uint8_t NK_get_last_command_status() {
		auto _copy = NK_last_command_status;
		NK_last_command_status = 0;
		return _copy;
	}

	NK_C_API int NK_login(const char *device_model) {
		auto m = NitrokeyManager::instance();
		try {
			NK_last_command_status = 0;
			return m->connect(device_model);
		}
		catch (CommandFailedException & commandFailedException) {
			NK_last_command_status = commandFailedException.last_command_status;
			return commandFailedException.last_command_status;
		}
    catch (const DeviceCommunicationException &deviceException){
      NK_last_command_status = 256-deviceException.getType();
      cerr << deviceException.what() << endl;
      return 0;
    }
		catch (std::runtime_error &e) {
			cerr << e.what() << endl;
			return 0;
		}
		return 0;
	}

        NK_C_API int NK_login_enum(NK_device_model device_model) {
                const char *model_string;
                switch (device_model) {
                    case NK_PRO:
                        model_string = "P";
                        break;
                    case NK_STORAGE:
                        model_string = "S";
                        break;
                    default:
                        /* no such enum value -- return error code */
                        return 0;
                }
                return NK_login(model_string);
        }

	NK_C_API int NK_logout() {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->disconnect();
		});
	}

	NK_C_API int NK_first_authenticate(const char* admin_password, const char* admin_temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			return m->first_authenticate(admin_password, admin_temporary_password);
		});
	}


	NK_C_API int NK_user_authenticate(const char* user_password, const char* user_temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->user_authenticate(user_password, user_temporary_password);
		});
	}

	NK_C_API int NK_factory_reset(const char* admin_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->factory_reset(admin_password);
		});
	}
	NK_C_API int NK_build_aes_key(const char* admin_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->build_aes_key(admin_password);
		});
	}

	NK_C_API int NK_unlock_user_password(const char *admin_password, const char *new_user_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->unlock_user_password(admin_password, new_user_password);
		});
	}

	NK_C_API int NK_write_config(uint8_t numlock, uint8_t capslock, uint8_t scrolllock, bool enable_user_password,
		bool delete_user_password,
		const char *admin_temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			return m->write_config(numlock, capslock, scrolllock, enable_user_password, delete_user_password, admin_temporary_password);
		});
	}


	NK_C_API uint8_t* NK_read_config() {
		auto m = NitrokeyManager::instance();
		return get_with_array_result([&]() {
			auto v = m->read_config();
			return duplicate_vector_and_clear(v);
		});
	}


	void clear_string(std::string &s) {
		std::fill(s.begin(), s.end(), ' ');
	}


	NK_C_API const char * NK_status() {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			string && s = m->get_status_as_string();
			char * rs = strndup(s.c_str(), max_string_field_length);
			clear_string(s);
			return rs;
		});
	}

	NK_C_API const char * NK_device_serial_number() {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			string && s = m->get_serial_number();
			char * rs = strndup(s.c_str(), max_string_field_length);
			clear_string(s);
			return rs;
		});
	}

	NK_C_API const char * NK_get_hotp_code(uint8_t slot_number) {
		return NK_get_hotp_code_PIN(slot_number, "");
	}

	NK_C_API const char * NK_get_hotp_code_PIN(uint8_t slot_number, const char *user_temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			string && s = m->get_HOTP_code(slot_number, user_temporary_password);
			char * rs = strndup(s.c_str(), max_string_field_length);
			clear_string(s);
			return rs;
		});
	}

	NK_C_API const char * NK_get_totp_code(uint8_t slot_number, uint64_t challenge, uint64_t last_totp_time,
		uint8_t last_interval) {
		return NK_get_totp_code_PIN(slot_number, challenge, last_totp_time, last_interval, "");
	}

	NK_C_API const char * NK_get_totp_code_PIN(uint8_t slot_number, uint64_t challenge, uint64_t last_totp_time,
		uint8_t last_interval, const char *user_temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			string && s = m->get_TOTP_code(slot_number, challenge, last_totp_time, last_interval, user_temporary_password);
			char * rs = strndup(s.c_str(), max_string_field_length);
			clear_string(s);
			return rs;
		});
	}

	NK_C_API int NK_erase_hotp_slot(uint8_t slot_number, const char *temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&] {
			m->erase_hotp_slot(slot_number, temporary_password);
		});
	}

	NK_C_API int NK_erase_totp_slot(uint8_t slot_number, const char *temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&] {
			m->erase_totp_slot(slot_number, temporary_password);
		});
	}

	NK_C_API int NK_write_hotp_slot(uint8_t slot_number, const char *slot_name, const char *secret, uint64_t hotp_counter,
		bool use_8_digits, bool use_enter, bool use_tokenID, const char *token_ID,
		const char *temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&] {
			m->write_HOTP_slot(slot_number, slot_name, secret, hotp_counter, use_8_digits, use_enter, use_tokenID, token_ID,
				temporary_password);
		});
	}

	NK_C_API int NK_write_totp_slot(uint8_t slot_number, const char *slot_name, const char *secret, uint16_t time_window,
		bool use_8_digits, bool use_enter, bool use_tokenID, const char *token_ID,
		const char *temporary_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&] {
			m->write_TOTP_slot(slot_number, slot_name, secret, time_window, use_8_digits, use_enter, use_tokenID, token_ID,
				temporary_password);
		});
	}

	NK_C_API const char* NK_get_totp_slot_name(uint8_t slot_number) {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			const auto slot_name = m->get_totp_slot_name(slot_number);
			return slot_name;
		});
	}
	NK_C_API const char* NK_get_hotp_slot_name(uint8_t slot_number) {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			const auto slot_name = m->get_hotp_slot_name(slot_number);
			return slot_name;
		});
	}

	NK_C_API void NK_set_debug(bool state) {
		auto m = NitrokeyManager::instance();
		m->set_debug(state);
	}


	NK_C_API void NK_set_debug_level(const int level) {
		auto m = NitrokeyManager::instance();
		m->set_loglevel(level);
	}

	NK_C_API int NK_totp_set_time(uint64_t time) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->set_time(time);
		});
	}

	NK_C_API int NK_totp_get_time() {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->get_time(0); // FIXME check how that should work
		});
	}

	NK_C_API int NK_change_admin_PIN(const char *current_PIN, const char *new_PIN) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->change_admin_PIN(current_PIN, new_PIN);
		});
	}

	NK_C_API int NK_change_user_PIN(const char *current_PIN, const char *new_PIN) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->change_user_PIN(current_PIN, new_PIN);
		});
	}

	NK_C_API int NK_enable_password_safe(const char *user_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->enable_password_safe(user_pin);
		});
	}
	NK_C_API uint8_t * NK_get_password_safe_slot_status() {
		auto m = NitrokeyManager::instance();
		return get_with_array_result([&]() {
			auto slot_status = m->get_password_safe_slot_status();
			return duplicate_vector_and_clear(slot_status);
		});

	}

	NK_C_API uint8_t NK_get_user_retry_count() {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return m->get_user_retry_count();
		});
	}

	NK_C_API uint8_t NK_get_admin_retry_count() {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return m->get_admin_retry_count();
		});
	}

	NK_C_API int NK_lock_device() {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->lock_device();
		});
	}

	NK_C_API const char *NK_get_password_safe_slot_name(uint8_t slot_number) {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			return m->get_password_safe_slot_name(slot_number);
		});
	}

	NK_C_API const char *NK_get_password_safe_slot_login(uint8_t slot_number) {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			return m->get_password_safe_slot_login(slot_number);
		});
	}
	NK_C_API const char *NK_get_password_safe_slot_password(uint8_t slot_number) {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			return m->get_password_safe_slot_password(slot_number);
		});
	}
	NK_C_API int NK_write_password_safe_slot(uint8_t slot_number, const char *slot_name, const char *slot_login,
		const char *slot_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->write_password_safe_slot(slot_number, slot_name, slot_login, slot_password);
		});
	}

	NK_C_API int NK_erase_password_safe_slot(uint8_t slot_number) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->erase_password_safe_slot(slot_number);
		});
	}

	NK_C_API int NK_is_AES_supported(const char *user_password) {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return (uint8_t)m->is_AES_supported(user_password);
		});
	}

	NK_C_API int NK_login_auto() {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return (uint8_t)m->connect();
		});
	}

	// storage commands

	NK_C_API int NK_send_startup(uint64_t seconds_from_epoch) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->send_startup(seconds_from_epoch);
		});
	}

	NK_C_API int NK_unlock_encrypted_volume(const char* user_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->unlock_encrypted_volume(user_pin);
		});
	}

	NK_C_API int NK_lock_encrypted_volume() {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->lock_encrypted_volume();
		});
	}

	NK_C_API int NK_unlock_hidden_volume(const char* hidden_volume_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->unlock_hidden_volume(hidden_volume_password);
		});
	}

	NK_C_API int NK_lock_hidden_volume() {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->lock_hidden_volume();
		});
	}

	NK_C_API int NK_create_hidden_volume(uint8_t slot_nr, uint8_t start_percent, uint8_t end_percent,
		const char *hidden_volume_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->create_hidden_volume(slot_nr, start_percent, end_percent,
				hidden_volume_password);
		});
	}

	NK_C_API int NK_set_unencrypted_read_only(const char *user_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->set_unencrypted_read_only(user_pin);
		});
	}

	NK_C_API int NK_set_unencrypted_read_write(const char *user_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->set_unencrypted_read_write(user_pin);
		});
	}

	NK_C_API int NK_set_unencrypted_read_only_admin(const char *admin_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->set_unencrypted_read_only_admin(admin_pin);
		});
	}

	NK_C_API int NK_set_unencrypted_read_write_admin(const char *admin_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->set_unencrypted_read_write_admin(admin_pin);
		});
	}

	NK_C_API int NK_set_encrypted_read_only(const char* admin_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->set_encrypted_volume_read_only(admin_pin);
		});
	}

	NK_C_API int NK_set_encrypted_read_write(const char* admin_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->set_encrypted_volume_read_write(admin_pin);
		});
	}

	NK_C_API int NK_export_firmware(const char* admin_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->export_firmware(admin_pin);
		});
	}

	NK_C_API int NK_clear_new_sd_card_warning(const char* admin_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->clear_new_sd_card_warning(admin_pin);
		});
	}

	NK_C_API int NK_fill_SD_card_with_random_data(const char* admin_pin) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->fill_SD_card_with_random_data(admin_pin);
		});
	}

	NK_C_API int NK_change_update_password(const char* current_update_password,
		const char* new_update_password) {
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->change_update_password(current_update_password, new_update_password);
		});
	}

	NK_C_API int NK_enable_firmware_update(const char* update_password){
		auto m = NitrokeyManager::instance();
		return get_without_result([&]() {
			m->enable_firmware_update(update_password);
		});
	}

	NK_C_API const char* NK_get_status_storage_as_string() {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			return m->get_status_storage_as_string();
		});
	}

	NK_C_API const char* NK_get_SD_usage_data_as_string() {
		auto m = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			return m->get_SD_usage_data_as_string();
		});
	}

	NK_C_API int NK_get_progress_bar_value() {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return m->get_progress_bar_value();
		});
	}

	NK_C_API int NK_get_major_firmware_version() {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return m->get_major_firmware_version();
		});
	}

  NK_C_API int NK_get_minor_firmware_version() {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return m->get_minor_firmware_version();
		});
	}

  NK_C_API int NK_set_unencrypted_volume_rorw_pin_type_user() {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return m->set_unencrypted_volume_rorw_pin_type_user() ? 1 : 0;
		});
	}

	NK_C_API const char* NK_list_devices_by_cpuID() {
		auto nm = NitrokeyManager::instance();
		return get_with_string_result([&]() {
			auto v = nm->list_devices_by_cpuID();
			std::string res;
			for (const auto a : v){
				res += a+";";
			}
			if (res.size()>0) res.pop_back(); // remove last delimiter char
			return strndup(res.c_str(), 8192); //this buffer size sets limit to over 200 devices ID's
		});
	}

	NK_C_API int NK_connect_with_ID(const char* id) {
		auto m = NitrokeyManager::instance();
		return get_with_result([&]() {
			return m->connect_with_ID(id) ? 1 : 0;
		});
	}



#ifdef __cplusplus
}
#endif
