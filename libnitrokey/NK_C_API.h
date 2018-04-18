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

#ifndef LIBNITROKEY_NK_C_API_H
#define LIBNITROKEY_NK_C_API_H

#include <stdbool.h>
#include <stdint.h>

#ifdef _MSC_VER
#define NK_C_API __declspec(dllexport)
#else
#define NK_C_API 
#endif

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * The Nitrokey device models supported by the API.
         */
        enum NK_device_model {
            /**
             * Nitrokey Pro.
             */
            NK_PRO,
            /**
             * Nitrokey Storage.
             */
            NK_STORAGE
        };

	/**
	 * Set debug level of messages written on stderr
	 * @param state state=True - most messages, state=False - only errors level
	 */
	NK_C_API void NK_set_debug(bool state);

	/**
	 * Set debug level of messages written on stderr
	 * @param level (int) 0-lowest verbosity, 5-highest verbosity
	 */
  NK_C_API void NK_set_debug_level(const int level);

	/**
	 * Connect to device of given model. Currently library can be connected only to one device at once.
	 * @param device_model char 'S': Nitrokey Storage, 'P': Nitrokey Pro
	 * @return 1 if connected, 0 if wrong model or cannot connect
	 */
	NK_C_API int NK_login(const char *device_model);

	/**
	 * Connect to device of given model. Currently library can be connected only to one device at once.
	 * @param device_model NK_device_model: NK_PRO: Nitrokey Pro, NK_STORAGE: Nitrokey Storage
	 * @return 1 if connected, 0 if wrong model or cannot connect
	 */
        NK_C_API int NK_login_enum(enum NK_device_model device_model);

	/**
	 * Connect to first available device, starting checking from Pro 1st to Storage 2nd.
	 * @return 1 if connected, 0 if wrong model or cannot connect
	 */
	NK_C_API int NK_login_auto();

	/**
	 * Disconnect from the device.
	 * @return command processing error code
	 */
	NK_C_API int NK_logout();

	/**
	 * Return the debug status string. Debug purposes.
	 * @return command processing error code
	 */
	NK_C_API const char * NK_status();

	/**
	 * Return the device's serial number string in hex.
	 * @return string device's serial number in hex
	 */
	NK_C_API const char * NK_device_serial_number();

	/**
	 * Get last command processing status. Useful for commands which returns the results of their own and could not return
	 * an error code.
	 * @return previous command processing error code
	 */
	NK_C_API uint8_t NK_get_last_command_status();

	/**
	 * Lock device - cancel any user device unlocking.
	 * @return command processing error code
	 */
	NK_C_API int NK_lock_device();

	/**
	 * Authenticates the user on USER privilages with user_password and sets user's temporary password on device to user_temporary_password.
	 * @param user_password char[25](Pro) current user password
	 * @param user_temporary_password char[25](Pro) user temporary password to be set on device for further communication (authentication command)
	 * @return command processing error code
	 */
	NK_C_API int NK_user_authenticate(const char* user_password, const char* user_temporary_password);

	/**
	 * Authenticates the user on ADMIN privilages with admin_password and sets user's temporary password on device to admin_temporary_password.
	 * @param admin_password char[25](Pro) current administrator PIN
	 * @param admin_temporary_password char[25](Pro) admin temporary password to be set on device for further communication (authentication command)
	 * @return command processing error code
	 */
	NK_C_API int NK_first_authenticate(const char* admin_password, const char* admin_temporary_password);

	/**
	 * Execute a factory reset.
	 * @param admin_password char[20](Pro) current administrator PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_factory_reset(const char* admin_password);

	/**
	 * Generates AES key on the device
	 * @param admin_password char[20](Pro) current administrator PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_build_aes_key(const char* admin_password);

	/**
	 * Unlock user PIN locked after 3 incorrect codes tries.
	 * @param admin_password char[20](Pro) current administrator PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_unlock_user_password(const char *admin_password, const char *new_user_password);

	/**
	 * Write general config to the device
	 * @param numlock set value in range [0-1] to send HOTP code from slot 'numlock' after double pressing numlock
	 * or outside the range to disable this function
	 * @param capslock similar to numlock but with capslock
	 * @param scrolllock similar to numlock but with scrolllock
	 * @param enable_user_password set True to enable OTP PIN protection (require PIN each OTP code request)
	 * @param delete_user_password (unused)
	 * @param admin_temporary_password current admin temporary password
	 * @return command processing error code
	 */
	NK_C_API int NK_write_config(uint8_t numlock, uint8_t capslock, uint8_t scrolllock,
		bool enable_user_password, bool delete_user_password, const char *admin_temporary_password);

	/**
	 * Get currently set config - status of function Numlock/Capslock/Scrollock OTP sending and is enabled PIN protected OTP
	 * @see NK_write_config
	 * @return  uint8_t general_config[5]:
	 *            uint8_t numlock;
				  uint8_t capslock;
				  uint8_t scrolllock;
				  uint8_t enable_user_password;
				  uint8_t delete_user_password;

	 */
	NK_C_API uint8_t* NK_read_config();

	//OTP

	/**
	 * Get name of given TOTP slot
	 * @param slot_number TOTP slot number, slot_number<15
	 * @return char[20](Pro) the name of the slot
	 */
	NK_C_API const char * NK_get_totp_slot_name(uint8_t slot_number);

	/**
	 *
	 * @param slot_number HOTP slot number, slot_number<3
	 * @return char[20](Pro) the name of the slot
	 */
	NK_C_API const char * NK_get_hotp_slot_name(uint8_t slot_number);

	/**
	 * Erase HOTP slot data from the device
	 * @param slot_number HOTP slot number, slot_number<3
	 * @param temporary_password admin temporary password
	 * @return command processing error code
	 */
	NK_C_API int NK_erase_hotp_slot(uint8_t slot_number, const char *temporary_password);

	/**
	 * Erase TOTP slot data from the device
	 * @param slot_number TOTP slot number, slot_number<15
	 * @param temporary_password admin temporary password
	 * @return command processing error code
	 */
	NK_C_API int NK_erase_totp_slot(uint8_t slot_number, const char *temporary_password);

	/**
	 * Write HOTP slot data to the device
	 * @param slot_number HOTP slot number, slot_number<3
	 * @param slot_name char[15](Pro) desired slot name
	 * @param secret char[20](Pro) 160-bit secret
	 * @param hotp_counter uint32_t starting value of HOTP counter
	 * @param use_8_digits should returned codes be 6 (false) or 8 digits (true)
	 * @param use_enter press ENTER key after sending OTP code using double-pressed scroll/num/capslock
	 * @param use_tokenID @see token_ID
	 * @param token_ID @see https://openauthentication.org/token-specs/, 'Class A' section
	 * @param temporary_password char[25](Pro) admin temporary password
	 * @return command processing error code
	 */
	NK_C_API int NK_write_hotp_slot(uint8_t slot_number, const char *slot_name, const char *secret, uint64_t hotp_counter,
		bool use_8_digits, bool use_enter, bool use_tokenID, const char *token_ID,
		const char *temporary_password);

	/**
	 * Write TOTP slot data to the device
	 * @param slot_number TOTP slot number, slot_number<15
	 * @param slot_name char[15](Pro) desired slot name
	 * @param secret char[20](Pro) 160-bit secret
	 * @param time_window uint16_t time window for this TOTP
	 * @param use_8_digits should returned codes be 6 (false) or 8 digits (true)
	 * @param use_enter press ENTER key after sending OTP code using double-pressed scroll/num/capslock
	 * @param use_tokenID @see token_ID
	 * @param token_ID @see https://openauthentication.org/token-specs/, 'Class A' section
	 * @param temporary_password char[20](Pro) admin temporary password
	 * @return command processing error code
	 */
	NK_C_API int NK_write_totp_slot(uint8_t slot_number, const char *slot_name, const char *secret, uint16_t time_window,
		bool use_8_digits, bool use_enter, bool use_tokenID, const char *token_ID,
		const char *temporary_password);

	/**
	 * Get HOTP code from the device
	 * @param slot_number HOTP slot number, slot_number<3
	 * @return HOTP code
	 */
	NK_C_API const char * NK_get_hotp_code(uint8_t slot_number);

	/**
	 * Get HOTP code from the device (PIN protected)
	 * @param slot_number HOTP slot number, slot_number<3
	 * @param user_temporary_password char[25](Pro) user temporary password if PIN protected OTP codes are enabled,
	 * otherwise should be set to empty string - ''
	 * @return HOTP code
	 */
	NK_C_API const char * NK_get_hotp_code_PIN(uint8_t slot_number, const char *user_temporary_password);

	/**
	 * Get TOTP code from the device
	 * @param slot_number TOTP slot number, slot_number<15
	 * @param challenge TOTP challenge
	 * @param last_totp_time last time
	 * @param last_interval last interval
	 * @return TOTP code
	 */
	NK_C_API const char * NK_get_totp_code(uint8_t slot_number, uint64_t challenge, uint64_t last_totp_time,
		uint8_t last_interval);

	/**
	 * Get TOTP code from the device (PIN protected)
	 * @param slot_number TOTP slot number, slot_number<15
	 * @param challenge TOTP challenge
	 * @param last_totp_time last time
	 * @param last_interval last interval
	 * @param user_temporary_password char[25](Pro) user temporary password if PIN protected OTP codes are enabled,
	 * otherwise should be set to empty string - ''
	 * @return TOTP code
	 */
	NK_C_API const char * NK_get_totp_code_PIN(uint8_t slot_number, uint64_t challenge,
		uint64_t last_totp_time, uint8_t last_interval,
		const char *user_temporary_password);

	/**
	 * Set time on the device (for TOTP requests)
	 * @param time seconds in unix epoch (from 01.01.1970)
	 * @return command processing error code
	 */
	NK_C_API int NK_totp_set_time(uint64_t time);

	NK_C_API int NK_totp_get_time();

	//passwords
	/**
	 * Change administrator PIN
	 * @param current_PIN char[25](Pro) current PIN
	 * @param new_PIN char[25](Pro) new PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_change_admin_PIN(const char *current_PIN, const char *new_PIN);

	/**
	 * Change user PIN
	 * @param current_PIN char[25](Pro) current PIN
	 * @param new_PIN char[25](Pro) new PIN
	 * @return command processing error code
	*/
	NK_C_API int NK_change_user_PIN(const char *current_PIN, const char *new_PIN);


	/**
	 * Get retry count of user PIN
	 * @return user PIN retry count
	 */
	NK_C_API uint8_t NK_get_user_retry_count();

	/**
	 * Get retry count of admin PIN
	 * @return admin PIN retry count
	 */
	NK_C_API uint8_t NK_get_admin_retry_count();
	//password safe

	/**
	 * Enable password safe access
	 * @param user_pin char[30](Pro) current user PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_enable_password_safe(const char *user_pin);

	/**
	 * Get password safe slots' status
	 * @return uint8_t[16] slot statuses - each byte represents one slot with 0 (not programmed) and 1 (programmed)
	 */
	NK_C_API uint8_t * NK_get_password_safe_slot_status();

	/**
	 * Get password safe slot name
	 * @param slot_number password safe slot number, slot_number<16
	 * @return slot name
	 */
	NK_C_API const char *NK_get_password_safe_slot_name(uint8_t slot_number);

	/**
	 * Get password safe slot login
	 * @param slot_number password safe slot number, slot_number<16
	 * @return login from the PWS slot
	 */
	NK_C_API const char *NK_get_password_safe_slot_login(uint8_t slot_number);

	/**
	 * Get the password safe slot password
	 * @param slot_number password safe slot number, slot_number<16
	 * @return password from the PWS slot
	 */
	NK_C_API const char *NK_get_password_safe_slot_password(uint8_t slot_number);

	/**
	 * Write password safe data to the slot
	 * @param slot_number password safe slot number, slot_number<16
	 * @param slot_name char[11](Pro) name of the slot
	 * @param slot_login char[32](Pro) login string
	 * @param slot_password char[20](Pro) password string
	 * @return command processing error code
	 */
	NK_C_API int NK_write_password_safe_slot(uint8_t slot_number, const char *slot_name,
		const char *slot_login, const char *slot_password);

	/**
	 * Erase the password safe slot from the device
	 * @param slot_number password safe slot number, slot_number<16
	 * @return command processing error code
	 */
	NK_C_API int NK_erase_password_safe_slot(uint8_t slot_number);

	/**
	 * Check whether AES is supported by the device
	 * @return 0 for no and 1 for yes
	 */
	NK_C_API int NK_is_AES_supported(const char *user_password);

	/**
	 * Get device's major firmware version
	 * @return major part of the version number (e.g. 0 from 0.48, 0 from 0.7 etc.)
	 */
	NK_C_API int NK_get_major_firmware_version();

	/**
	 * Get device's minor firmware version
	 * @return minor part of the version number (e.g. 7 from 0.7, 48 from 0.48 etc.)
	 */
	NK_C_API int NK_get_minor_firmware_version();

  /**
   * Function to determine unencrypted volume PIN type
   * @param minor_firmware_version
   * @return Returns 1, if set unencrypted volume ro/rw pin type is User, 0 otherwise.
   */
	NK_C_API int NK_set_unencrypted_volume_rorw_pin_type_user();


	/**
	 * This command is typically run to initiate
	 * communication with the device (altough not required).
	 * It sets time on device and returns its current status
	 * - a combination of set_time and get_status_storage commands
	 * Storage only
	 * @param seconds_from_epoch date and time expressed in seconds
	 */
	NK_C_API int NK_send_startup(uint64_t seconds_from_epoch);

	/**
	 * Unlock encrypted volume.
	 * Storage only
	 * @param user_pin user pin 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_unlock_encrypted_volume(const char* user_pin);

	/**
	 * Locks encrypted volume
	 * @return command processing error code
	 */
	NK_C_API int NK_lock_encrypted_volume();

	/**
	 * Unlock hidden volume and lock encrypted volume.
	 * Requires encrypted volume to be unlocked.
	 * Storage only
	 * @param hidden_volume_password 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_unlock_hidden_volume(const char* hidden_volume_password);

	/**
	 * Locks hidden volume
	 * @return command processing error code
	 */
	NK_C_API int NK_lock_hidden_volume();

	/**
	 * Create hidden volume.
	 * Requires encrypted volume to be unlocked.
	 * Storage only
	 * @param slot_nr slot number in range 0-3
	 * @param start_percent volume begin expressed in percent of total available storage, int in range 0-99
	 * @param end_percent volume end expressed in percent of total available storage, int in range 1-100
	 * @param hidden_volume_password 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_create_hidden_volume(uint8_t slot_nr, uint8_t start_percent, uint8_t end_percent,
		const char *hidden_volume_password);

	/**
	 * Make unencrypted volume read-only.
	 * Device hides unencrypted volume for a second therefore make sure
	 * buffers are flushed before running.
	 * Does nothing if firmware version is not matched
	 * Firmware range: Storage v0.50, v0.48 and below
	 * Storage only
	 * @param user_pin 20 characters User PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_set_unencrypted_read_only(const char *user_pin);

	/**
	 * Make unencrypted volume read-write.
	 * Device hides unencrypted volume for a second therefore make sure
	 * buffers are flushed before running.
	 * Does nothing if firmware version is not matched
	 * Firmware range: Storage v0.50, v0.48 and below
	 * Storage only
	 * @param user_pin 20 characters User PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_set_unencrypted_read_write(const char *user_pin);

	/**
	 * Make unencrypted volume read-only.
	 * Device hides unencrypted volume for a second therefore make sure
	 * buffers are flushed before running.
	 * Does nothing if firmware version is not matched
	 * Firmware range: Storage v0.49, v0.51+
	 * Storage only
	 * @param admin_pin 20 characters Admin PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_set_unencrypted_read_only_admin(const char* admin_pin);

	/**
	 * Make unencrypted volume read-write.
	 * Device hides unencrypted volume for a second therefore make sure
	 * buffers are flushed before running.
	 * Does nothing if firmware version is not matched
	 * Firmware range: Storage v0.49, v0.51+
	 * Storage only
	 * @param admin_pin 20 characters Admin PIN
	 * @return command processing error code
	 */
	NK_C_API int NK_set_unencrypted_read_write_admin(const char* admin_pin);

	/**
	 * Make encrypted volume read-only.
	 * Device hides encrypted volume for a second therefore make sure
	 * buffers are flushed before running.
	 * Firmware range: v0.49 only, future (see firmware release notes)
	 * Storage only
	 * @param admin_pin 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_set_encrypted_read_only(const char* admin_pin);

	/**
	 * Make encrypted volume read-write.
	 * Device hides encrypted volume for a second therefore make sure
	 * buffers are flushed before running.
	 * Firmware range: v0.49 only, future (see firmware release notes)
	 * Storage only
	 * @param admin_pin 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_set_encrypted_read_write(const char* admin_pin);

	/**
	 * Exports device's firmware to unencrypted volume.
	 * Storage only
	 * @param admin_pin 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_export_firmware(const char* admin_pin);

	/**
	 * Clear new SD card notification. It is set after factory reset.
	 * Storage only
	 * @param admin_pin 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_clear_new_sd_card_warning(const char* admin_pin);

	/**
	 * Fill SD card with random data.
	 * Should be done on first stick initialization after creating keys.
	 * Storage only
	 * @param admin_pin 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_fill_SD_card_with_random_data(const char* admin_pin);

	/**
	 * Change update password.
	 * Update password is used for entering update mode, where firmware
	 * could be uploaded using dfu-programmer or other means.
	 * Storage only
	 * @param current_update_password 20 characters
	 * @param new_update_password 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_change_update_password(const char* current_update_password,
		const char* new_update_password);

	/**
	 * Enter update mode. Needs update password.
	 * When device is in update mode it no longer accepts any HID commands until
	 * firmware is launched (regardless of being updated or not).
	 * Smartcard (through CCID interface) and its all volumes are not visible as well.
	 * Its VID and PID are changed to factory-default (03eb:2ff1 Atmel Corp.)
	 * to be detected by flashing software. Result of this command can be reversed
	 * by using 'launch' command.
	 * For dfu-programmer it would be: 'dfu-programmer at32uc3a3256s launch'.
	 * Storage only
	 * @param update_password 20 characters
	 * @return command processing error code
	 */
	NK_C_API int NK_enable_firmware_update(const char* update_password);

	/**
	 * Get Storage stick status as string.
	 * Storage only
	 * @return string with devices attributes
	 */
	NK_C_API const char* NK_get_status_storage_as_string();

	/**
	 * Get SD card usage attributes as string.
	 * Usable during hidden volumes creation.
	 * Storage only
	 * @return string with SD card usage attributes
	 */
	NK_C_API const char* NK_get_SD_usage_data_as_string();

	/**
	 * Get progress value of current long operation.
	 * Storage only
	 * @return int in range 0-100 or -1 if device is not busy
	 */
	NK_C_API int NK_get_progress_bar_value();

/**
 * Returns a list of connected devices' id's, delimited by ';' character. Empty string is returned on no device found.
 * Each ID could consist of:
 * 1. SC_id:SD_id_p_path (about 40 bytes)
 * 2. path (about 10 bytes)
 * where 'path' is USB path (bus:num), 'SC_id' is smartcard ID, 'SD_id' is storage card ID and
 * '_p_' and ':' are field delimiters.
 * Case 2 (USB path only) is used, when the device cannot be asked about its status data (e.g. during a long operation,
 * like clearing SD card.
 * Internally connects to all available devices and creates a map between ids and connection objects.
 * Side effects: changes active device to last detected Storage device.
 * Storage only
 * @example Example of returned data: '00005d19:dacc2cb4_p_0001:0010:02;000037c7:4cf12445_p_0001:000f:02;0001:000c:02'
 * @return string delimited id's of connected devices
 */
	NK_C_API const char* NK_list_devices_by_cpuID();


/**
 * Connects to the device with given ID. ID's list could be created with NK_list_devices_by_cpuID.
 * Requires calling to NK_list_devices_by_cpuID first. Connecting to arbitrary ID/USB path is not handled.
 * On connection requests status from device and disconnects it / removes from map on connection failure.
 * Storage only
 * @param id Target device ID (example: '00005d19:dacc2cb4_p_0001:0010:02')
 * @return 1 on successful connection, 0 otherwise
 */
	NK_C_API int NK_connect_with_ID(const char* id);



#ifdef __cplusplus
}
#endif

#endif //LIBNITROKEY_NK_C_API_H
