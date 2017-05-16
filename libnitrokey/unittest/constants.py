from enum import Enum
from misc import to_hex

RFC_SECRET_HR = '12345678901234567890'
RFC_SECRET = to_hex(RFC_SECRET_HR)  # '31323334353637383930...'


# print( repr((RFC_SECRET, RFC_SECRET_, len(RFC_SECRET))) )

class DefaultPasswords(Enum):
    ADMIN = '12345678'
    USER = '123456'
    ADMIN_TEMP = '123123123'
    USER_TEMP = '234234234'
    UPDATE = '12345678'
    UPDATE_TEMP = '123update123'


class DeviceErrorCode(Enum):
    STATUS_OK = 0
    BUSY = 1 # busy or busy progressbar in place of wrong_CRC status
    NOT_PROGRAMMED = 3
    WRONG_PASSWORD = 4
    STATUS_NOT_AUTHORIZED = 5
    STATUS_AES_DEC_FAILED = 0xa


class LibraryErrors(Enum):
    TOO_LONG_STRING = 200
    INVALID_SLOT = 201
    INVALID_HEX_STRING = 202
    TARGET_BUFFER_SIZE_SMALLER_THAN_SOURCE = 203

