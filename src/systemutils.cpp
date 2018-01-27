/*
 * Copyright (c) 2016-2018 Nitrokey UG
 *
 * This file is part of Nitrokey App.
 *
 * Nitrokey App is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey App is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "systemutils.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

namespace systemutils {

bool isNitroDevice(string dev) {
  // returns true if under /sys/block/dev/vendor is name nitro
  const string nitroName = "NitroKey";
  try {
    string path = string("/sys/block/") + dev + string("/device/vendor");
    ifstream vendorInfo(path.c_str());
    string vendor;
    vendorInfo >> vendor;
    if (vendor.find(nitroName) != string::npos)
      return true;
  } catch (...) {
  }
  return false;
}

string getEncryptedDevice() {
  // return full encrypted device path
  // return empty string if one or none devices are connected
  // This approach would only work if encrypted device has letter after
  // not encrypted
  std::vector<string> devices, nitrodevices;

  ifstream partitionsFile("/proc/partitions");
  string minor, major, size, device;
  while (partitionsFile) {
    partitionsFile >> major >> minor >> size >> device;
    if (isdigit(*(device.end() - 1)))
      continue;
    devices.push_back(device);
  }

  for (size_t i = 0; i < devices.size(); ++i) {
    if (isNitroDevice(devices[i]))
      nitrodevices.push_back(devices[i]);
  }

  if (nitrodevices.size() <= 1)
    return "";
  sort(nitrodevices.begin(), nitrodevices.end());
  return string("/dev/") + nitrodevices.back();
}

string getMntPoint(string) {
  return string();
}
}
