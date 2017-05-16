//
// Created by sz on 23.07.16.
//

#ifndef LIBNITROKEY_COMMANDFAILEDEXCEPTION_H
#define LIBNITROKEY_COMMANDFAILEDEXCEPTION_H

#include <exception>
#include <cstdint>
#include "log.h"
#include "command_id.h"

using cs = nitrokey::proto::stick10::command_status;

class CommandFailedException : public std::exception {
public:
    const uint8_t last_command_id;
    const uint8_t last_command_status;

    CommandFailedException(uint8_t last_command_id, uint8_t last_command_status) :
        last_command_id(last_command_id),
            last_command_status(last_command_status){
      LOG(std::string("CommandFailedException, status: ")+ std::to_string(last_command_status), nitrokey::log::Loglevel::DEBUG);
    }

    virtual const char *what() const throw() {
        return "Command execution has failed on device";
    }


    bool reason_timestamp_warning() const throw(){
      return last_command_status == static_cast<uint8_t>(cs::timestamp_warning);
    }

    bool reason_AES_not_initialized() const throw(){
      return last_command_status == static_cast<uint8_t>(cs::AES_dec_failed);
    }

    bool reason_not_authorized() const throw(){
      return last_command_status == static_cast<uint8_t>(cs::not_authorized);
    }

    bool reason_slot_not_programmed() const throw(){
      return last_command_status == static_cast<uint8_t>(cs::slot_not_programmed);
    }

    bool reason_wrong_password() const throw(){
      return last_command_status == static_cast<uint8_t>(cs::wrong_password);
    }

};


#endif //LIBNITROKEY_COMMANDFAILEDEXCEPTION_H
