import pytest

from conftest import skip_if_device_version_lower_than
from constants import DefaultPasswords, DeviceErrorCode, RFC_SECRET, bb, bbRFC_SECRET
from misc import ffi, gs, wait, cast_pointer_to_tuple
from misc import is_pro_rtm_07, is_pro_rtm_08, is_storage

@pytest.mark.lock_device
@pytest.mark.PWS
def test_enable_password_safe(C):
    """
    All Password Safe tests depend on AES keys being initialized. They will fail otherwise.
    """
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert C.NK_enable_password_safe(b'wrong_password') == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK

@pytest.mark.lock_device
@pytest.mark.PWS
def test_write_password_safe_slot(C):
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert C.NK_write_password_safe_slot(0, b'slotname1', b'login1', b'pass1') == DeviceErrorCode.STATUS_NOT_AUTHORIZED
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_password_safe_slot(0, b'slotname1', b'login1', b'pass1') == DeviceErrorCode.STATUS_OK


@pytest.mark.lock_device
@pytest.mark.PWS
@pytest.mark.slowtest
def test_write_all_password_safe_slots_and_read_10_times(C):
    def fill(s, wid):
        assert wid >= len(s)
        numbers = '1234567890'*4
        s += numbers[:wid-len(s)]
        assert len(s) == wid
        return bb(s)

    def get_pass(suffix):
        return fill('pass' + suffix, 20)

    def get_loginname(suffix):
        return fill('login' + suffix, 32)

    def get_slotname(suffix):
        return fill('slotname' + suffix, 11)

    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    PWS_slot_count = 16
    for i in range(0, PWS_slot_count):
        iss = str(i)
        assert C.NK_write_password_safe_slot(i,
                                             get_slotname(iss), get_loginname(iss),
                                             get_pass(iss)) == DeviceErrorCode.STATUS_OK

    for j in range(0, 10):
        for i in range(0, PWS_slot_count):
            iss = str(i)
            assert gs(C.NK_get_password_safe_slot_name(i)) == get_slotname(iss)
            assert gs(C.NK_get_password_safe_slot_login(i)) == get_loginname(iss)
            assert gs(C.NK_get_password_safe_slot_password(i)) == get_pass(iss)


@pytest.mark.lock_device
@pytest.mark.PWS
@pytest.mark.slowtest
@pytest.mark.xfail(reason="This test should be run directly after test_write_all_password_safe_slots_and_read_10_times")
def test_read_all_password_safe_slots_10_times(C):
    def fill(s, wid):
        assert wid >= len(s)
        numbers = '1234567890'*4
        s += numbers[:wid-len(s)]
        assert len(s) == wid
        return bb(s)

    def get_pass(suffix):
        return fill('pass' + suffix, 20)

    def get_loginname(suffix):
        return fill('login' + suffix, 32)

    def get_slotname(suffix):
        return fill('slotname' + suffix, 11)

    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    PWS_slot_count = 16

    for j in range(0, 10):
        for i in range(0, PWS_slot_count):
            iss = str(i)
            assert gs(C.NK_get_password_safe_slot_name(i)) == get_slotname(iss)
            assert gs(C.NK_get_password_safe_slot_login(i)) == get_loginname(iss)
            assert gs(C.NK_get_password_safe_slot_password(i)) == get_pass(iss)


@pytest.mark.lock_device
@pytest.mark.PWS
def test_get_password_safe_slot_name(C):
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_password_safe_slot(0, b'slotname1', b'login1', b'pass1') == DeviceErrorCode.STATUS_OK
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert gs(C.NK_get_password_safe_slot_name(0)) == b''
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_NOT_AUTHORIZED

    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert gs(C.NK_get_password_safe_slot_name(0)) == b'slotname1'
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK


@pytest.mark.PWS
def test_get_password_safe_slot_login_password(C):
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_password_safe_slot(0, b'slotname1', b'login1', b'pass1') == DeviceErrorCode.STATUS_OK
    slot_login = C.NK_get_password_safe_slot_login(0)
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK
    assert gs(slot_login) == b'login1'
    slot_password = gs(C.NK_get_password_safe_slot_password(0))
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK
    assert slot_password == b'pass1'


@pytest.mark.PWS
def test_erase_password_safe_slot(C):
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_erase_password_safe_slot(0) == DeviceErrorCode.STATUS_OK
    assert gs(C.NK_get_password_safe_slot_name(0)) == b''
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK  # TODO CHECK shouldn't this be DeviceErrorCode.NOT_PROGRAMMED ?


@pytest.mark.PWS
def test_password_safe_slot_status(C):
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_erase_password_safe_slot(0) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_password_safe_slot(1, b'slotname2', b'login2', b'pass2') == DeviceErrorCode.STATUS_OK
    safe_slot_status = C.NK_get_password_safe_slot_status()
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK
    is_slot_programmed = list(ffi.cast("uint8_t [16]", safe_slot_status)[0:16])
    print((is_slot_programmed, len(is_slot_programmed)))
    assert is_slot_programmed[0] == 0
    assert is_slot_programmed[1] == 1


@pytest.mark.aes
def test_issue_device_locks_on_second_key_generation_in_sequence(C):
    if is_pro_rtm_07(C) or is_pro_rtm_08(C):
        pytest.skip("issue to register: device locks up "
                     "after below commands sequence (reinsertion fixes), skipping for now")
    assert C.NK_build_aes_key(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    assert C.NK_build_aes_key(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK


@pytest.mark.aes
def test_regenerate_aes_key(C):
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_build_aes_key(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK


@pytest.mark.lock_device
@pytest.mark.aes
@pytest.mark.factory_reset
def test_enable_password_safe_after_factory_reset(C):
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    if is_storage(C):
        # for some reason storage likes to be authenticated before reset (to investigate)
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_factory_reset(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    wait(10)
    if is_storage(C):
        assert C.NK_clear_new_sd_card_warning(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    enable_password_safe_result = C.NK_enable_password_safe(DefaultPasswords.USER)
    assert enable_password_safe_result == DeviceErrorCode.STATUS_AES_DEC_FAILED \
           or is_storage(C) and enable_password_safe_result == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_build_aes_key(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK

@pytest.mark.lock_device
@pytest.mark.aes
@pytest.mark.xfail(reason="NK Pro firmware bug: regenerating AES key command not always results in cleared slot data")
def test_destroy_password_safe(C):
    """
    Sometimes fails on NK Pro - slot name is not cleared ergo key generation has not succeed despite the success result
    returned from the device
    """
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    # write password safe slot
    assert C.NK_write_password_safe_slot(0, b'slotname1', b'login1', b'pass1') == DeviceErrorCode.STATUS_OK
    # read slot
    assert gs(C.NK_get_password_safe_slot_name(0)) == b'slotname1'
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK
    slot_login = C.NK_get_password_safe_slot_login(0)
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK
    assert gs(slot_login) == b'login1'
    # destroy password safe by regenerating aes key
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK

    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_build_aes_key(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK

    assert gs(C.NK_get_password_safe_slot_name(0)) != b'slotname1'
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK

    # check was slot status cleared
    safe_slot_status = C.NK_get_password_safe_slot_status()
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK
    is_slot_programmed = list(ffi.cast("uint8_t [16]", safe_slot_status)[0:16])
    assert is_slot_programmed[0] == 0


@pytest.mark.aes
def test_is_AES_supported(C):
    if is_storage(C):
        pytest.skip("Storage does not implement this command")
    assert C.NK_is_AES_supported(b'wrong password') != 1
    assert C.NK_get_last_command_status() == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_is_AES_supported(DefaultPasswords.USER) == 1
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK


@pytest.mark.pin
def test_admin_PIN_change(C):
    new_password = b'123123123'
    assert C.NK_change_admin_PIN(b'wrong_password', new_password) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_change_admin_PIN(DefaultPasswords.ADMIN, new_password) == DeviceErrorCode.STATUS_OK
    assert C.NK_change_admin_PIN(new_password, DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK


@pytest.mark.pin
def test_user_PIN_change(C):
    new_password = b'123123123'
    assert C.NK_change_user_PIN(b'wrong_password', new_password) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_change_user_PIN(DefaultPasswords.USER, new_password) == DeviceErrorCode.STATUS_OK
    assert C.NK_change_user_PIN(new_password, DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK


@pytest.mark.lock_device
@pytest.mark.pin
def test_admin_retry_counts(C):
    default_admin_retry_count = 3
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert C.NK_get_admin_retry_count() == default_admin_retry_count
    assert C.NK_change_admin_PIN(b'wrong_password', DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_get_admin_retry_count() == default_admin_retry_count - 1
    assert C.NK_change_admin_PIN(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    assert C.NK_get_admin_retry_count() == default_admin_retry_count


@pytest.mark.lock_device
@pytest.mark.pin
def test_user_retry_counts_change_PIN(C):
    assert C.NK_change_user_PIN(DefaultPasswords.USER, DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    wrong_password = b'wrong_password'
    default_user_retry_count = 3
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert C.NK_get_user_retry_count() == default_user_retry_count
    assert C.NK_change_user_PIN(wrong_password, wrong_password) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_get_user_retry_count() == default_user_retry_count - 1
    assert C.NK_change_user_PIN(DefaultPasswords.USER, DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_get_user_retry_count() == default_user_retry_count


@pytest.mark.lock_device
@pytest.mark.pin
def test_user_retry_counts_PWSafe(C):
    default_user_retry_count = 3
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    assert C.NK_get_user_retry_count() == default_user_retry_count
    assert C.NK_enable_password_safe(b'wrong_password') == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_get_user_retry_count() == default_user_retry_count - 1
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_get_user_retry_count() == default_user_retry_count


@pytest.mark.pin
def test_unlock_user_password(C):
    default_user_retry_count = 3
    default_admin_retry_count = 3
    new_password = b'123123123'
    assert C.NK_get_user_retry_count() == default_user_retry_count
    assert C.NK_change_user_PIN(b'wrong_password', new_password) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_change_user_PIN(b'wrong_password', new_password) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_change_user_PIN(b'wrong_password', new_password) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_get_user_retry_count() == default_user_retry_count - 3
    assert C.NK_get_admin_retry_count() == default_admin_retry_count

    assert C.NK_unlock_user_password(b'wrong password', DefaultPasswords.USER) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_get_admin_retry_count() == default_admin_retry_count - 1
    assert C.NK_unlock_user_password(DefaultPasswords.ADMIN, DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_get_user_retry_count() == default_user_retry_count
    assert C.NK_get_admin_retry_count() == default_admin_retry_count


@pytest.mark.pin
def test_admin_auth(C):
    assert C.NK_first_authenticate(b'wrong_password', DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK


@pytest.mark.pin
def test_user_auth(C):
    assert C.NK_user_authenticate(b'wrong_password', DefaultPasswords.USER_TEMP) == DeviceErrorCode.WRONG_PASSWORD
    assert C.NK_user_authenticate(DefaultPasswords.USER, DefaultPasswords.USER_TEMP) == DeviceErrorCode.STATUS_OK


@pytest.mark.otp
def check_HOTP_RFC_codes(C, func, prep=None, use_8_digits=False):
    """
    # https://tools.ietf.org/html/rfc4226#page-32
    """
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_hotp_slot(1, b'python_test', bbRFC_SECRET, 0, use_8_digits, False, False, b'',
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    test_data = [
        1284755224, 1094287082, 137359152, 1726969429, 1640338314, 868254676, 1918287922, 82162583, 673399871,
        645520489,
    ]
    for code in test_data:
        if prep:
            prep()
        r = func(1)
        code = str(code)[-8:] if use_8_digits else str(code)[-6:]
        assert bb(code) == r


@pytest.mark.otp
@pytest.mark.parametrize("use_8_digits", [False, True, ])
@pytest.mark.parametrize("use_pin_protection", [False, True, ])
def test_HOTP_RFC_use8digits_usepin(C, use_8_digits, use_pin_protection):
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, use_pin_protection, not use_pin_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    if use_pin_protection:
        check_HOTP_RFC_codes(C,
                             lambda x: gs(C.NK_get_hotp_code_PIN(x, DefaultPasswords.USER_TEMP)),
                             lambda: C.NK_user_authenticate(DefaultPasswords.USER, DefaultPasswords.USER_TEMP),
                             use_8_digits=use_8_digits)
    else:
        check_HOTP_RFC_codes(C, lambda x: gs(C.NK_get_hotp_code(x)), use_8_digits=use_8_digits)


@pytest.mark.otp
def test_HOTP_token(C):
    """
    Check HOTP routine with written token ID to slot.
    """
    use_pin_protection = False
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, use_pin_protection, not use_pin_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    token_ID = b"AAV100000022"
    assert C.NK_write_hotp_slot(1, b'python_test', bbRFC_SECRET, 0, False, False, True, token_ID,
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    for i in range(5):
        hotp_code = gs(C.NK_get_hotp_code(1))
        assert hotp_code != b''
        assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK


@pytest.mark.otp
def test_HOTP_counters(C):
    """
    # https://tools.ietf.org/html/rfc4226#page-32
    """
    use_pin_protection = False
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, use_pin_protection, not use_pin_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    use_8_digits = True
    HOTP_test_data = [
        1284755224, 1094287082, 137359152, 1726969429, 1640338314,
        868254676, 1918287922, 82162583, 673399871, 645520489,
    ]
    slot_number = 1
    for counter, code in enumerate(HOTP_test_data):
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_write_hotp_slot(slot_number, b'python_test', bbRFC_SECRET, counter, use_8_digits, False, False, b'',
                                    DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        r = gs(C.NK_get_hotp_code(slot_number))
        code = str(code)[-8:] if use_8_digits else str(code)[-6:]
        assert bb(code) == r


INT32_MAX = 2 ** 31 - 1
@pytest.mark.otp
def test_HOTP_64bit_counter(C):
    if is_storage(C):
        pytest.xfail('bug in NK Storage HOTP firmware - counter is set with a 8 digits string, '
                     'however int32max takes 10 digits to be written')
    oath = pytest.importorskip("oath")
    lib_at = lambda t: bb(oath.hotp(RFC_SECRET, t, format='dec6'))
    PIN_protection = False
    use_8_digits = False
    slot_number = 1
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    dev_res = []
    lib_res = []
    for t in range(INT32_MAX - 5, INT32_MAX + 5, 1):
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_write_hotp_slot(slot_number, b'python_test', bbRFC_SECRET, t, use_8_digits, False, False, b'',
                                    DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        code_device = gs(C.NK_get_hotp_code(slot_number))
        dev_res += (t, code_device)
        lib_res += (t, lib_at(t))
    assert dev_res == lib_res


@pytest.mark.otp
def test_TOTP_64bit_time(C):
    if is_storage(C):
        pytest.xfail('bug in NK Storage TOTP firmware')
    oath = pytest.importorskip("oath")
    T = 1
    lib_at = lambda t: bb(oath.totp(RFC_SECRET, t=t))
    PIN_protection = False
    slot_number = 1
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_totp_slot(slot_number, b'python_test', bbRFC_SECRET, 30, False, False, False, b'',
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    dev_res = []
    lib_res = []
    for t in range(INT32_MAX - 5, INT32_MAX + 5, 1):
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_totp_set_time(t) == DeviceErrorCode.STATUS_OK
        code_device = gs((C.NK_get_totp_code(slot_number, T, 0, 30)))
        dev_res += (t, code_device)
        lib_res += (t, lib_at(t))
    assert dev_res == lib_res


@pytest.mark.otp
@pytest.mark.xfail(reason="NK Pro: Test fails in 50% of cases due to test vectors set 1 second before interval count change"
                          "Here time is changed on seconds side only and miliseconds part is not being reset apparently"
                          "This results in available time to test of half a second on average, thus 50% failed cases"
                          "With disabled two first test vectors test passess 10/10 times"
                          "Fail may also occurs on NK Storage with lower occurrency since it needs less time to execute "
                          "commands")
@pytest.mark.parametrize("PIN_protection", [False, True, ])
def test_TOTP_RFC_usepin(C, PIN_protection):
    slot_number = 1
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    # test according to https://tools.ietf.org/html/rfc6238#appendix-B
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_totp_slot(slot_number, b'python_test', bbRFC_SECRET, 30, True, False, False, b'',
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK

    get_func = None
    if PIN_protection:
        get_func = lambda x, y, z, r: gs(C.NK_get_totp_code_PIN(x, y, z, r, DefaultPasswords.USER_TEMP))
    else:
        get_func = lambda x, y, z, r: gs(C.NK_get_totp_code(x, y, z, r))

    # Mode: Sha1, time step X=30
    test_data = [
        #Time         T (hex)               TOTP
        (59,          0x1,                94287082), # Warning - test vector time 1 second before interval count changes
        (1111111109,  0x00000000023523EC, 7081804), # Warning - test vector time 1 second before interval count changes
        (1111111111,  0x00000000023523ED, 14050471),
        (1234567890,  0x000000000273EF07, 89005924),
        (2000000000,  0x0000000003F940AA, 69279037),
        # (20000000000, 0x0000000027BC86AA, 65353130), # 64bit is also checked in other test
    ]
    responses = []
    data = []
    correct = 0
    for t, T, expected_code in test_data:
        if PIN_protection:
            C.NK_user_authenticate(DefaultPasswords.USER, DefaultPasswords.USER_TEMP)
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_totp_set_time(t) == DeviceErrorCode.STATUS_OK
        code_from_device = get_func(slot_number, T, 0, 30)  # FIXME T is not changing the outcome
        data += [ (t, bb(str(expected_code).zfill(8))) ]
        responses += [ (t, code_from_device) ]
        correct += expected_code == code_from_device
    assert data == responses or correct == len(test_data)


@pytest.mark.otp
def test_get_slot_names(C):
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_erase_totp_slot(0, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    # erasing slot invalidates temporary password, so requesting authentication
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_erase_hotp_slot(0, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK

    for i in range(15):
        name = ffi.string(C.NK_get_totp_slot_name(i))
        if name == '':
            assert C.NK_get_last_command_status() == DeviceErrorCode.NOT_PROGRAMMED
    for i in range(3):
        name = ffi.string(C.NK_get_hotp_slot_name(i))
        if name == '':
            assert C.NK_get_last_command_status() == DeviceErrorCode.NOT_PROGRAMMED


@pytest.mark.otp
def test_get_OTP_codes(C):
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, False, True, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    for i in range(15):
        code = gs(C.NK_get_totp_code(i, 0, 0, 0))
        if code == b'':
            assert C.NK_get_last_command_status() == DeviceErrorCode.NOT_PROGRAMMED

    for i in range(3):
        code = gs(C.NK_get_hotp_code(i))
        if code == b'':
            assert C.NK_get_last_command_status() == DeviceErrorCode.NOT_PROGRAMMED


@pytest.mark.otp
def test_get_OTP_code_from_not_programmed_slot(C):
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, False, True, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_erase_hotp_slot(0, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_erase_totp_slot(0, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK

    code = gs(C.NK_get_hotp_code(0))
    assert code == b''
    assert C.NK_get_last_command_status() == DeviceErrorCode.NOT_PROGRAMMED

    code = gs(C.NK_get_totp_code(0, 0, 0, 0))
    assert code == b''
    assert C.NK_get_last_command_status() == DeviceErrorCode.NOT_PROGRAMMED


@pytest.mark.otp
def test_get_code_user_authorize(C):
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_totp_slot(0, b'python_otp_auth', bbRFC_SECRET, 30, True, False, False, b'',
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    # enable PIN protection of OTP codes with write_config
    # TODO create convinience function on C API side to enable/disable OTP USER_PIN protection
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, True, False, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    code = gs(C.NK_get_totp_code(0, 0, 0, 0))
    assert code == b''
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_NOT_AUTHORIZED
    # disable PIN protection with write_config
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, False, True, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    code = gs(C.NK_get_totp_code(0, 0, 0, 0))
    assert code != b''
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK


def cast_pointer_to_tuple(obj, typen, len):
    # usage:
    #     config = cast_pointer_to_tuple(config_raw_data, 'uint8_t', 5)
    return tuple(ffi.cast("%s [%d]" % (typen, len), obj)[0:len])


def test_read_write_config(C):
    # let's set sample config with pin protection and disabled scrolllock
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(0, 1, 2, True, False, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    config_raw_data = C.NK_read_config()
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK
    config = cast_pointer_to_tuple(config_raw_data, 'uint8_t', 5)
    assert config == (0, 1, 2, True, False)

    # restore defaults and check
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, False, True, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    config_raw_data = C.NK_read_config()
    assert C.NK_get_last_command_status() == DeviceErrorCode.STATUS_OK
    config = cast_pointer_to_tuple(config_raw_data, 'uint8_t', 5)
    assert config == (255, 255, 255, False, True)


@pytest.mark.lock_device
@pytest.mark.factory_reset
def test_factory_reset(C):
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, False, True, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_hotp_slot(1, b'python_test', bbRFC_SECRET, 0, False, False, False, b"",
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert gs(C.NK_get_hotp_code(1)) == b"755224"
    assert C.NK_factory_reset(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    wait(10)
    assert gs(C.NK_get_hotp_code(1)) != b"287082"
    assert C.NK_get_last_command_status() == DeviceErrorCode.NOT_PROGRAMMED
    # restore AES key
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_build_aes_key(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK
    assert C.NK_enable_password_safe(DefaultPasswords.USER) == DeviceErrorCode.STATUS_OK
    assert C.NK_lock_device() == DeviceErrorCode.STATUS_OK
    if is_storage(C):
       assert C.NK_clear_new_sd_card_warning(DefaultPasswords.ADMIN) == DeviceErrorCode.STATUS_OK


@pytest.mark.status
def test_get_status(C):
    status = C.NK_status()
    s = gs(status)
    assert len(s) > 0

@pytest.mark.status
def test_get_serial_number(C):
    sn = C.NK_device_serial_number()
    sn = gs(sn)
    assert len(sn) > 0
    print(('Serial number of the device: ', sn))


@pytest.mark.otp
@pytest.mark.parametrize("secret", ['000001', '00'*10+'ff', '00'*19+'ff', '000102',
                                    '00'*29+'ff', '00'*39+'ff', '002EF43F51AFA97BA2B46418768123C9E1809A5B' ])
def test_OTP_secret_started_from_null(C, secret):
    """
    NK Pro 0.8+, NK Storage 0.43+
    """
    skip_if_device_version_lower_than({'S': 43, 'P': 8})
    if len(secret) > 40:
        # feature: 320 bit long secret handling
        skip_if_device_version_lower_than({'P': 8})

    oath = pytest.importorskip("oath")
    lib_at = lambda t: bb(oath.hotp(secret, t, format='dec6'))
    PIN_protection = False
    use_8_digits = False
    slot_number = 1
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    dev_res = []
    lib_res = []
    for t in range(1,5):
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_write_hotp_slot(slot_number, b'null_secret', bb(secret), t, use_8_digits, False, False, b'',
                                    DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        code_device = gs(C.NK_get_hotp_code(slot_number))
        dev_res += (t, code_device)
        lib_res += (t, lib_at(t))
    assert dev_res == lib_res


@pytest.mark.otp
@pytest.mark.parametrize("counter", [0, 3, 7, 0xffff,
                                     0xffffffff,
                                     0xffffffffffffffff] )
def test_HOTP_slots_read_write_counter(C, counter):
    """
    Write different counters to all HOTP slots, read code and compare with 3rd party
    :param counter:
    """
    if counter >= 1e7:
        # Storage does not handle counters longer than 7 digits
        skip_if_device_version_lower_than({'P': 7})

    secret = RFC_SECRET
    oath = pytest.importorskip("oath")
    lib_at = lambda t: bb(oath.hotp(secret, t, format='dec6'))
    PIN_protection = False
    use_8_digits = False
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    dev_res = []
    lib_res = []
    for slot_number in range(3):
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_write_hotp_slot(slot_number, b'HOTP rw' + bytes(slot_number), bb(secret), counter, use_8_digits, False, False, b"",
                                    DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        code_device = gs(C.NK_get_hotp_code(slot_number))
        dev_res += (counter, code_device)
        lib_res += (counter, lib_at(counter))
    assert dev_res == lib_res


@pytest.mark.otp
@pytest.mark.parametrize("period", [30,60] )
@pytest.mark.parametrize("time", range(21,70,20) )
def test_TOTP_slots_read_write_at_time_period(C, time, period):
    """
    Write to all TOTP slots with specified period, read code at specified time
    and compare with 3rd party
    """
    secret = RFC_SECRET
    oath = pytest.importorskip("oath")
    lib_at = lambda t: bb(oath.totp(RFC_SECRET, t=t, period=period))
    PIN_protection = False
    use_8_digits = False
    T = 0
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    dev_res = []
    lib_res = []
    for slot_number in range(15):
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_write_totp_slot(slot_number, b'TOTP rw' + bytes(slot_number), bb(secret), period, use_8_digits, False, False, b"",
                                    DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_totp_set_time(time) == DeviceErrorCode.STATUS_OK
        code_device = gs(C.NK_get_totp_code(slot_number, T, 0, period))
        dev_res += (time, code_device)
        lib_res += (time, lib_at(time))
    assert dev_res == lib_res


@pytest.mark.otp
@pytest.mark.parametrize("secret", [RFC_SECRET, 2*RFC_SECRET, '12'*10, '12'*30] )
def test_TOTP_secrets(C, secret):
    '''
    NK Pro 0.8+, NK Storage 0.44+
    '''
    skip_if_device_version_lower_than({'S': 44, 'P': 8})

    if len(secret)>20*2: #*2 since secret is in hex
        # pytest.skip("Secret lengths over 20 bytes are not supported by NK Pro 0.7 and NK Storage")
        skip_if_device_version_lower_than({'P': 8})
    slot_number = 0
    time = 0
    period = 30
    oath = pytest.importorskip("oath")
    lib_at = lambda t: bb(oath.totp(secret, t=t, period=period))
    PIN_protection = False
    use_8_digits = False
    T = 0
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    dev_res = []
    lib_res = []
    assert C.NK_write_totp_slot(slot_number, b'secret' + bytes(len(secret)), bb(secret), period, use_8_digits, False, False, b"",
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_totp_set_time(time) == DeviceErrorCode.STATUS_OK
    code_device = gs(C.NK_get_totp_code(slot_number, T, 0, period))
    dev_res += (time, code_device)
    lib_res += (time, lib_at(time))
    assert dev_res == lib_res


@pytest.mark.otp
@pytest.mark.parametrize("secret", [RFC_SECRET, 2*RFC_SECRET, '12'*10, '12'*30] )
def test_HOTP_secrets(C, secret):
    """
    NK Pro 0.8+
    feature needed: support for 320bit secrets
    """
    if len(secret)>40:
        skip_if_device_version_lower_than({'P': 8})

    slot_number = 0
    counter = 0
    oath = pytest.importorskip("oath")
    lib_at = lambda t: bb(oath.hotp(secret, counter=t))
    PIN_protection = False
    use_8_digits = False
    T = 0
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    dev_res = []
    lib_res = []
    # repeat authentication for Pro 0.7
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_hotp_slot(slot_number, b'secret' + bytes(len(secret)), bb(secret), counter, use_8_digits, False, False, b"",
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    code_device = gs(C.NK_get_hotp_code(slot_number))
    dev_res += (counter, code_device)
    lib_res += (counter, lib_at(counter))
    assert dev_res == lib_res


def test_special_double_press(C):
    """
    requires manual check after function run
    double press each of num-, scroll-, caps-lock and check inserted OTP codes (each 1st should be 755224)
    on nkpro 0.7 scrolllock should do nothing, on nkpro 0.8+ should return OTP code
    """
    secret = RFC_SECRET
    counter = 0
    PIN_protection = False
    use_8_digits = False
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(0, 1, 2, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    for slot_number in range(3):
        assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
        assert C.NK_write_hotp_slot(slot_number, b'double' + bytes(slot_number), bb(secret), counter, use_8_digits, False, False, b"",
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    # requires manual check


@pytest.mark.otp
def test_edit_OTP_slot(C):
    """
    should change slots counter and name without changing its secret (using null secret for second update)
    """
    # counter is not getting updated under Storage v0.43 - #TOREPORT
    skip_if_device_version_lower_than({'S': 44, 'P': 7})

    secret = RFC_SECRET
    counter = 0
    PIN_protection = False
    use_8_digits = False
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    slot_number = 0
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    first_name = b'edit slot'
    assert C.NK_write_hotp_slot(slot_number, first_name, bb(secret), counter, use_8_digits, False, False, b"",
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert gs(C.NK_get_hotp_slot_name(slot_number)) == first_name


    first_code = gs(C.NK_get_hotp_code(slot_number))
    changed_name = b'changedname'
    empty_secret = b''
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_hotp_slot(slot_number, changed_name, empty_secret, counter, use_8_digits, False, False, b"",
                                DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    second_code = gs(C.NK_get_hotp_code(slot_number))
    assert first_code == second_code
    assert gs(C.NK_get_hotp_slot_name(slot_number)) == changed_name


@pytest.mark.otp
@pytest.mark.skip
@pytest.mark.parametrize("secret", ['31323334353637383930'*2,'31323334353637383930'*4] )
def test_TOTP_codes_from_nitrokeyapp(secret, C):
    """
    Helper test for manual TOTP check of written secret by Nitrokey App
    Destined to run by hand
    """
    slot_number = 0
    PIN_protection = False
    period = 30
    assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    assert C.NK_write_config(255, 255, 255, PIN_protection, not PIN_protection,
                             DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    code_device = gs(C.NK_get_totp_code(slot_number, 0, 0, period))

    oath = pytest.importorskip("oath")
    lib_at = lambda : bb(oath.totp(secret, period=period))
    print (lib_at())
    assert lib_at() == code_device
