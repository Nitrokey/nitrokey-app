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

import pytest

from misc import ffi

device_type = None

def skip_if_device_version_lower_than(allowed_devices):
    global device_type
    model, version = device_type
    infinite_version_number = 999
    if allowed_devices.get(model, infinite_version_number) > version:
        pytest.skip('This device model is not applicable to run this test')


@pytest.fixture(scope="module")
def C(request=None):
    fp = '../NK_C_API.h'

    declarations = []
    with open(fp, 'r') as f:
        declarations = f.readlines()

    cnt = 0
    a = iter(declarations)
    for declaration in a:
        if 'NK_device_model' in declaration: continue
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
    path_build = os.path.join("..", "build")
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
        sys.exit(1)

    C.NK_set_debug_level(int(os.environ.get('LIBNK_DEBUG', 2)))

    nk_login = C.NK_login_auto()
    if nk_login != 1:
        print('No devices detected!')
    assert nk_login != 0  # returns 0 if not connected or wrong model or 1 when connected
    global device_type
    firmware_version = C.NK_get_minor_firmware_version()
    model = 'P' if firmware_version in [7,8] else 'S'
    device_type = (model, firmware_version)
    print('Connected device: {} {}'.format(model, firmware_version))

    # assert C.NK_first_authenticate(DefaultPasswords.ADMIN, DefaultPasswords.ADMIN_TEMP) == DeviceErrorCode.STATUS_OK
    # assert C.NK_user_authenticate(DefaultPasswords.USER, DefaultPasswords.USER_TEMP) == DeviceErrorCode.STATUS_OK

    # C.NK_status()

    def fin():
        print('\nFinishing connection to device')
        C.NK_logout()
        print('Finished')

    if request:
        request.addfinalizer(fin)
    # C.NK_set_debug(True)
    C.NK_set_debug_level(int(os.environ.get('LIBNK_DEBUG', 3)))

    return C
