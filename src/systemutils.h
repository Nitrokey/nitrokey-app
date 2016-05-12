#include <string>
namespace systemutils {
bool isNitroDevice(std::string dev);
std::string getUnencryptedDevice();
std::string getEncryptedDevice();
std::string getMntPoint(std::string deviceName);
}
