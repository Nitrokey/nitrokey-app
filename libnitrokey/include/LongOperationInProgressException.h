//
// Created by sz on 24.10.16.
//

#ifndef LIBNITROKEY_LONGOPERATIONINPROGRESSEXCEPTION_H
#define LIBNITROKEY_LONGOPERATIONINPROGRESSEXCEPTION_H

#include "CommandFailedException.h"

class LongOperationInProgressException : public CommandFailedException {

public:
    unsigned char progress_bar_value;

    LongOperationInProgressException(
        unsigned char _command_id, uint8_t last_command_status, unsigned char _progress_bar_value)
    : CommandFailedException(_command_id, last_command_status), progress_bar_value(_progress_bar_value){
      LOG(
          std::string("LongOperationInProgressException, progress bar status: ")+
              std::to_string(progress_bar_value), nitrokey::log::Loglevel::DEBUG);
    }
    virtual const char *what() const throw() {
      return "Device returned busy status with long operation in progress";
    }
};


#endif //LIBNITROKEY_LONGOPERATIONINPROGRESSEXCEPTION_H
