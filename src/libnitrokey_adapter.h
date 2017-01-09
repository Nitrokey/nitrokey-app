//
// Created by sz on 09.01.17.
//

#ifndef NITROKEYAPP_LIBNITROKEY_ADAPTER_H
#define NITROKEYAPP_LIBNITROKEY_ADAPTER_H

#include <memory>
#include <string>


class libnitrokey_adapter {
  private:
    libnitrokey_adapter();
    static std::shared_ptr <libnitrokey_adapter> _instance;

public:
    ~libnitrokey_adapter();
    static std::shared_ptr<libnitrokey_adapter> instance();

    int getMajorFirmwareVersion();
    int getMinorFirmwareVersion();
    int getPasswordRetryCount();
    int getUserPasswordRetryCount();
    std::string getCardSerial();
    int getStorageInfoData();
    int getStorageSDCardSize();

    bool isDeviceConnected();
    bool isStorageDeviceConnected();
};


#endif //NITROKEYAPP_LIBNITROKEY_ADAPTER_H
