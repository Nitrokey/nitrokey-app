"""
Copyright (c) 2017-2018 Nitrokey UG

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

import pprint
from time import sleep

import pytest
from collections import defaultdict

from tqdm import tqdm

from conftest import skip_if_device_version_lower_than
from constants import DefaultPasswords, DeviceErrorCode, bb
from misc import gs, wait

pprint = pprint.PrettyPrinter(indent=4).pprint


@pytest.mark.other
@pytest.mark.info
def test_get_status_storage_multiple(C):
    ids = gs(C.NK_list_devices_by_cpuID())
    print(ids)
    devices_list = ids.split(b';')
    #
    # for s in devices_list:
    #     res = C.NK_connect_with_ID(s)
    #     assert res == 1
    #     # st = gs(C.NK_get_status_storage_as_string())
    #     # print(len(st))
    #     C.NK_lock_device()
    #
    for s in devices_list:
        res = C.NK_connect_with_ID(s)
        assert res == 1
        v = C.NK_fill_SD_card_with_random_data(b'12345678')
        # print(v)

    devices_count = len(devices_list)
    assert devices_count != 0
    b = 0

    last_b = 0
    with tqdm(total=devices_count*100) as pbar:
        while b/devices_count < 100:
            pbar.update(b - last_b)

            b = defaultdict (lambda: 0)

            ids = gs(C.NK_list_devices_by_cpuID())
            devices_list = ids.split(b';')
            devices_count = len(devices_list)

            for s in devices_list:
                res = C.NK_connect_with_ID(s)
                if res != 1: continue
                b[s] += C.NK_get_progress_bar_value()
            print(b)
            b = sum(b.values())
            print('{}: {}'.format(b, int(b/devices_count) * '='))
            sleep(5)


