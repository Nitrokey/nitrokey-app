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

