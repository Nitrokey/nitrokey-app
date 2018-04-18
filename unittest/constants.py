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

from misc import to_hex

def bb(x):
    return bytes(x, encoding='ascii')


RFC_SECRET_HR = '12345678901234567890'
RFC_SECRET = to_hex(RFC_SECRET_HR)  # '31323334353637383930...'
bbRFC_SECRET = bb(RFC_SECRET)


# print( repr((RFC_SECRET, RFC_SECRET_, len(RFC_SECRET))) )

class DefaultPasswords:
    ADMIN = b'12345678'
    USER = b'123456'
    ADMIN_TEMP = b'123123123'
    USER_TEMP = b'234234234'
    UPDATE = b'12345678'
    UPDATE_TEMP = b'123update123'


class DeviceErrorCode:
    STATUS_OK = 0
    BUSY = 1 # busy or busy progressbar in place of wrong_CRC status
    NOT_PROGRAMMED = 3
    WRONG_PASSWORD = 4
    STATUS_NOT_AUTHORIZED = 5
    STATUS_AES_DEC_FAILED = 0xa


class LibraryErrors:
    TOO_LONG_STRING = 200
    INVALID_SLOT = 201
    INVALID_HEX_STRING = 202
    TARGET_BUFFER_SIZE_SMALLER_THAN_SOURCE = 203

