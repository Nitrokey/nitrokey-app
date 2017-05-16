import pytest

from misc import ffi, gs, to_hex, is_pro_rtm_07, is_long_OTP_secret_handled
from constants import DefaultPasswords, DeviceErrorCode, RFC_SECRET, LibraryErrors


def test_too_long_strings(C):
    new_password = '123123123'
    long_string = 'a' * 100
    assert C.NK_change_user_PIN(long_string, new_password) == LibraryErrors.TOO_LONG_STRING
    assert C.NK_change_user_PIN(new_password, long_string) == LibraryErrors.TOO_LONG_STRING
    assert C.NK_change_admin_PIN(long_string, new_password) == LibraryErrors.TOO_LONG_STRING
    assert C.NK_change_admin_PIN(new_password, long_string) == LibraryErrors.TOO_LONG_STRING
    assert C.NK_first_authenticate(long_string, DefaultPasswords.ADMIN_TEMP) == LibraryErrors.TOO_LONG_STRING
    assert C.NK_erase_totp_slot(0, long_string) == LibraryErrors.TOO_LONG_STRING
    digits = False
    assert C.NK_write_hotp_slot(1, long_string, RFC_SECRET, 0, digits, False, False, "",
                                DefaultPasswords.ADMIN_TEMP) == LibraryErrors.TOO_LONG_STRING
    assert C.NK_write_hotp_slot(1, 'long_test', RFC_SECRET, 0, digits, False, False, "",
                                long_string) == LibraryErrors.TOO_LONG_STRING
    assert C.NK_get_hotp_code_PIN(0, long_string) == 0
    assert C.NK_get_last_command_status() == LibraryErrors.TOO_LONG_STRING


def test_invalid_slot(C):
    invalid_slot = 255
    assert C.NK_erase_totp_slot(invalid_slot, 'some password') == LibraryErrors.INVALID_SLOT
    assert C.NK_write_hotp_slot(invalid_slot, 'long_test', RFC_SECRET, 0, False, False, False, "",
                                'aaa') == LibraryErrors.INVALID_SLOT
    assert C.NK_get_hotp_code_PIN(invalid_slot, 'some password') == 0
    assert C.NK_get_last_command_status() == LibraryErrors.INVALID_SLOT
    assert C.NK_erase_password_safe_slot(invalid_slot) == LibraryErrors.INVALID_SLOT
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert gs(C.NK_get_password_safe_slot_name(invalid_slot)) == ''
    assert C.NK_get_last_command_status() == LibraryErrors.INVALID_SLOT
    assert gs(C.NK_get_password_safe_slot_login(invalid_slot)) == ''
    assert C.NK_get_last_command_status() == LibraryErrors.INVALID_SLOT


@pytest.mark.parametrize("invalid_hex_string",
                         ['text', '00  ', '0xff', 'zzzzzzzzzzzz', 'fff', 'f' * 257, 'f' * 258])
def test_invalid_secret_hex_string_for_OTP_write(C, invalid_hex_string):
    """
    Tests for invalid secret hex string during writing to OTP slot. Invalid strings are not hexadecimal number,
    empty or longer than 255 characters.
    """
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_hotp_slot(1, 'slot_name', invalid_hex_string, 0, True, False, False, '',
                                DefaultPasswords.ADMIN_TEMP) == LibraryErrors.INVALID_HEX_STRING
    assert C.NK_write_totp_slot(1, 'python_test', invalid_hex_string, 30, True, False, False, "",
                                DefaultPasswords.ADMIN_TEMP) == LibraryErrors.INVALID_HEX_STRING

def test_warning_binary_bigger_than_secret_buffer(C):
    invalid_hex_string = to_hex('1234567890') * 3
    if is_long_OTP_secret_handled(C):
        invalid_hex_string *= 2
    assert C.NK_write_hotp_slot(1, 'slot_name', invalid_hex_string, 0, True, False, False, '',
                                DefaultPasswords.ADMIN_TEMP) == LibraryErrors.TARGET_BUFFER_SIZE_SMALLER_THAN_SOURCE


@pytest.mark.skip(reason='Experimental')
def test_clear(C):
    d = 'asdasdasd'
    print(d)
    C.clear_password(d)
    print(d)