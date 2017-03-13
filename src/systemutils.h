#pragma once

#include <string>
namespace systemutils {
  bool isNitroDevice(std::string dev);
  std::string getEncryptedDevice();
  std::string getMntPoint(std::string deviceName);
}
