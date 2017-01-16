#include "systemutils.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <mntent.h>
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

string getMntPoint(string deviceName) {
  // return mount point of given device
  string res;
  FILE *src = setmntent("/etc/mtab", "r");
  struct mntent mnt;
  char buf[4096];
  while (getmntent_r(src, &mnt, buf, sizeof(buf))) {
    if (mnt.mnt_dir == NULL)
      continue;
    string device(mnt.mnt_fsname);
    if (device.find(deviceName) != string::npos) {
      res = string(mnt.mnt_dir);
      break;
    }
  }
  endmntent(src);
  return res;
}
}
