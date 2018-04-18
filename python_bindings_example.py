#!/usr/bin/env python2
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
from enum import Enum

"""
This example will print 10 HOTP codes from just written HOTP#2 slot.
For more examples of use please refer to unittest/test_*.py files.
"""

ffi = cffi.FFI()
get_string = ffi.string

class DeviceErrorCode(Enum):
    STATUS_OK = 0
    NOT_PROGRAMMED = 3
    WRONG_PASSWORD = 4
    STATUS_NOT_AUTHORIZED = 5
    STATUS_AES_DEC_FAILED = 0xa


def get_library():
    fp = 'NK_C_API.h'  # path to C API header

    declarations = []
    with open(fp, 'r') as f:
        declarations = f.readlines()

    cnt = 0
    a = iter(declarations)
    for declaration in a:
        if declaration.strip().startswith('NK_C_API'):
            declaration = declaration.replace('NK_C_API', '').strip()
            while ';' not in declaration:
                declaration += (next(a)).strip()
            # print(declaration)
            ffi.cdef(declaration, override=True)
            cnt +=1
    print('Imported {} declarations'.format(cnt))


    C = None
    import os, sys
    path_build = os.path.join(".", "build")
    paths = [
            os.environ.get('LIBNK_PATH', None),
            os.path.join(path_build,"libnitrokey.so"),
            os.path.join(path_build,"libnitrokey.dylib"),
            os.path.join(path_build,"libnitrokey.dll"),
            os.path.join(path_build,"nitrokey.dll"),
    ]
    for p in paths:
        if not p: continue
        print("Trying " +p)
        p = os.path.abspath(p)
        if os.path.exists(p):
            print("Found: "+p)
            C = ffi.dlopen(p)
            break
        else:
            print("File does not exist: " + p)
    if not C:
        print("No library file found")
        print("Please set the path using LIBNK_PATH environment variable to existing library or compile it (see "
              "README.md for details)")
        sys.exit(1)

    return C


def get_hotp_code(lib, i):
    return get_string(lib.NK_get_hotp_code(i))

def to_hex(ss):
    return ''.join([ format(ord(s),'02x') for s in ss ])

print('Warning!')
print('This example will change your configuration on inserted stick and overwrite your HOTP#2 slot.')
print('Please write "continue" to continue or any other string to quit')
a = raw_input()

if not a == 'continue':
    exit()

ADMIN = raw_input('Please enter your admin PIN (empty string uses 12345678) ')
ADMIN = ADMIN or '12345678'  # use default if empty string

show_log = raw_input('Should log messages be shown (please write "yes" to enable; this will make harder reading script output) ') == 'yes'
libnitrokey = get_library()

if show_log:
    log_level = raw_input('Please select verbosity level (0-5, 2 is library default, 3 will be selected on empty input) ')
    log_level = log_level or '3'
    log_level = int(log_level)
    libnitrokey.NK_set_debug_level(log_level)
else:
    libnitrokey.NK_set_debug_level(2)


ADMIN_TEMP = '123123123'
RFC_SECRET = to_hex('12345678901234567890')

# libnitrokey.NK_login('S')  # connect only to Nitrokey Storage device
# libnitrokey.NK_login('P')  # connect only to Nitrokey Pro device
device_connected = libnitrokey.NK_login_auto()  # connect to any Nitrokey Stick
if device_connected:
    print('Connected to Nitrokey device!')
else:
    print('Could not connect to Nitrokey device!')
    exit()
use_8_digits = True
pin_correct = libnitrokey.NK_first_authenticate(ADMIN, ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
if pin_correct:
    print('Your PIN is correct!')
else:
    print('Your PIN is not correct! Please try again. Please be careful to not lock your stick!')
    retry_count_left = libnitrokey.NK_get_admin_retry_count()
    print('Retry count left: %d' % retry_count_left )
    exit()

# For function parameters documentation please check NK_C_API.h
assert libnitrokey.NK_write_config(255, 255, 255, False, True, ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
libnitrokey.NK_first_authenticate(ADMIN, ADMIN_TEMP)
libnitrokey.NK_write_hotp_slot(1, 'python_test', RFC_SECRET, 0, use_8_digits, False, False, "",
                            ADMIN_TEMP)
# RFC test according to: https://tools.ietf.org/html/rfc4226#page-32
test_data = [
    1284755224, 1094287082, 137359152, 1726969429, 1640338314, 868254676, 1918287922, 82162583, 673399871,
    645520489,
]
print('Getting HOTP code from Nitrokey Stick (RFC test, 8 digits): ')
for i in range(10):
    hotp_slot_1_code = get_hotp_code(libnitrokey, 1)
    correct_str =  "correct!" if hotp_slot_1_code == str(test_data[i])[-8:] else  "not correct"
    print('%d: %s, should be %s -> %s' % (i, hotp_slot_1_code, str(test_data[i])[-8:], correct_str))
libnitrokey.NK_logout()  # disconnect device
