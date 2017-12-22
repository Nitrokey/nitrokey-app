#ifndef LIBNITROKEY_DEVICECOMMUNICATIONEXCEPTIONS_H
#define LIBNITROKEY_DEVICECOMMUNICATIONEXCEPTIONS_H

#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>


class DeviceCommunicationException: public std::runtime_error
{
  std::string message;
  static std::atomic_int occurred;
public:
  DeviceCommunicationException(std::string _msg): std::runtime_error(_msg), message(_msg){
    ++occurred;
  }
  uint8_t getType() const {return 1;};
//  virtual const char* what() const throw() override {
//    return message.c_str();
//  }
  static bool has_occurred(){ return occurred > 0; };
  static void reset_occurred_flag(){ occurred = 0; };
};

class DeviceNotConnected: public DeviceCommunicationException {
public:
  DeviceNotConnected(std::string msg) : DeviceCommunicationException(msg){}
  uint8_t getType() const {return 2;};
};

class DeviceSendingFailure: public DeviceCommunicationException {
public:
  DeviceSendingFailure(std::string msg) : DeviceCommunicationException(msg){}
  uint8_t getType() const {return 3;};
};

class DeviceReceivingFailure: public DeviceCommunicationException {
public:
  DeviceReceivingFailure(std::string msg) : DeviceCommunicationException(msg){}
  uint8_t getType() const {return 4;};
};

class InvalidCRCReceived: public DeviceReceivingFailure {
public:
  InvalidCRCReceived(std::string msg) : DeviceReceivingFailure(msg){}
  uint8_t getType() const {return 5;};
};


#endif //LIBNITROKEY_DEVICECOMMUNICATIONEXCEPTIONS_H
