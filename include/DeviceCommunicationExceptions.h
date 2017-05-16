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
//  virtual const char* what() const throw() override {
//    return message.c_str();
//  }
  static bool has_occurred(){ return occurred > 0; };
  static void reset_occurred_flag(){ occurred = 0; };
};

class DeviceNotConnected: public DeviceCommunicationException {
public:
  DeviceNotConnected(std::string msg) : DeviceCommunicationException(msg){}
};

class DeviceSendingFailure: public DeviceCommunicationException {
public:
  DeviceSendingFailure(std::string msg) : DeviceCommunicationException(msg){}
};

class DeviceReceivingFailure: public DeviceCommunicationException {
public:
  DeviceReceivingFailure(std::string msg) : DeviceCommunicationException(msg){}
};

class InvalidCRCReceived: public DeviceReceivingFailure {
public:
  InvalidCRCReceived(std::string msg) : DeviceReceivingFailure(msg){}
};


#endif //LIBNITROKEY_DEVICECOMMUNICATIONEXCEPTIONS_H
