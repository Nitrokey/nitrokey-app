import cffi

ffi = cffi.FFI()
gs = ffi.string


def to_hex(s):
    return "".join("{:02x}".format(ord(c)) for c in s)


def wait(t):
    import time
    msg = 'Waiting for %d seconds' % t
    print(msg.center(40, '='))
    time.sleep(t)


def cast_pointer_to_tuple(obj, typen, len):
    # usage:
    #     config = cast_pointer_to_tuple(config_raw_data, 'uint8_t', 5)
    return tuple(ffi.cast("%s [%d]" % (typen, len), obj)[0:len])


def get_devices_firmware_version(C):
    firmware = C.NK_get_major_firmware_version()
    return firmware


def is_pro_rtm_07(C):
    firmware = get_devices_firmware_version(C)
    return firmware == 7


def is_pro_rtm_08(C):
    firmware = get_devices_firmware_version(C)
    return firmware == 8


def is_storage(C):
    """
    exact firmware storage is sent by other function
    """
    # TODO identify connected device directly
    return not is_pro_rtm_08(C) and not is_pro_rtm_07(C)


def is_long_OTP_secret_handled(C):
    return is_pro_rtm_08(C) or is_storage(C) and get_devices_firmware_version(C) > 43
