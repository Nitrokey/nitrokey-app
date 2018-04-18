"""
Copyright (c) 2015-2018 Nitrokey UG

This file is part of libnitrokey.

libnitrokey is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

libnitrokey is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libnitrokey. If not, see <http://www.gnu.org/licenses/>.

SPDX-License-Identifier: LGPL-3.0
"""

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
    firmware = C.NK_get_minor_firmware_version()
    return firmware


def is_pro_rtm_07(C):
    firmware = get_devices_firmware_version(C)
    return firmware == 7


def is_pro_rtm_08(C):
    firmware = get_devices_firmware_version(C)
    return firmware in [8,9]


def is_storage(C):
    """
    exact firmware storage is sent by other function
    """
    # TODO identify connected device directly
    firmware = get_devices_firmware_version(C)
    return firmware >= 45


def is_long_OTP_secret_handled(C):
    return is_pro_rtm_08(C) or is_storage(C) and get_devices_firmware_version(C) > 43
