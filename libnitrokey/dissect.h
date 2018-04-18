/*
 * Copyright (c) 2015-2018 Nitrokey UG
 *
 * This file is part of libnitrokey.
 *
 * libnitrokey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * libnitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libnitrokey. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0
 */

/*
 *	Protocol packet dissection
 */
#ifndef DISSECT_H
#define DISSECT_H
#include <string>
#include <sstream>
#include <iomanip>
#include "misc.h"
#include "cxx_semantics.h"
#include "command_id.h"
#include "device_proto.h"

namespace nitrokey {
namespace proto {

template <CommandID id, class HIDPacket>
class QueryDissector : semantics::non_constructible {
 public:
  static std::string dissect(const HIDPacket &pod) {
    std::stringstream out;

#ifdef LOG_VOLATILE_DATA
    out << "Raw HID packet:" << std::endl;
    out << ::nitrokey::misc::hexdump((const uint8_t *)(&pod), sizeof pod);
#endif

    out << "Contents:" << std::endl;
    out << "Command ID:\t" << commandid_to_string((CommandID)(pod.command_id))
        << std::endl;
      out << "CRC:\t"
        << std::hex << std::setw(2) << std::setfill('0')
        << pod.crc << std::endl;

      out << "Payload:" << std::endl;
    out << pod.payload.dissect();
    return out.str();
  }
};




template <CommandID id, class HIDPacket>
class ResponseDissector : semantics::non_constructible {
 public:
    static std::string status_translate_device(int status){
      auto enum_status = static_cast<proto::stick10::device_status>(status);
      switch (enum_status){
        case stick10::device_status::ok: return "OK";
        case stick10::device_status::busy: return "BUSY";
        case stick10::device_status::error: return "ERROR";
        case stick10::device_status::received_report: return "RECEIVED_REPORT";
      }
      return std::string("UNKNOWN: ") + std::to_string(status);
    }

    static std::string to_upper(std::string str){
        for (auto & c: str) c = toupper(c);
      return str;
    }
    static std::string status_translate_command(int status){
      auto enum_status = static_cast<proto::stick10::command_status >(status);
      switch (enum_status) {
#define p(X) case X: return to_upper(std::string(#X));
        p(stick10::command_status::ok)
        p(stick10::command_status::wrong_CRC)
        p(stick10::command_status::wrong_slot)
        p(stick10::command_status::slot_not_programmed)
        p(stick10::command_status::wrong_password)
        p(stick10::command_status::not_authorized)
        p(stick10::command_status::timestamp_warning)
        p(stick10::command_status::no_name_error)
        p(stick10::command_status::not_supported)
        p(stick10::command_status::unknown_command)
        p(stick10::command_status::AES_dec_failed)
#undef p
      }
      return std::string("UNKNOWN: ") + std::to_string(status);
    }

  static std::string dissect(const HIDPacket &pod) {
    std::stringstream out;

    // FIXME use values from firmware (possibly generate separate
    // header automatically)

#ifdef LOG_VOLATILE_DATA
    out << "Raw HID packet:" << std::endl;
    out << ::nitrokey::misc::hexdump((const uint8_t *)(&pod), sizeof pod);
#endif

    out << "Device status:\t" << pod.device_status + 0 << " "
        << status_translate_device(pod.device_status) << std::endl;
    out << "Command ID:\t" << commandid_to_string((CommandID)(pod.command_id)) << " hex: " << std::hex << (int)pod.command_id
        << std::endl;
    out << "Last command CRC:\t"
            << std::hex << std::setw(2) << std::setfill('0')
            << pod.last_command_crc << std::endl;
    out << "Last command status:\t" << pod.last_command_status + 0 << " "
        << status_translate_command(pod.last_command_status) << std::endl;
    out << "CRC:\t"
            << std::hex << std::setw(2) << std::setfill('0')
            << pod.crc << std::endl;
    if((int)pod.command_id == pod.storage_status.command_id){
      out << "Storage stick status (where applicable):" << std::endl;
#define d(x) out << " "#x": \t"<< std::hex << std::setw(2) \
    << std::setfill('0')<< static_cast<int>(x) << std::endl;
    d(pod.storage_status.command_counter);
    d(pod.storage_status.command_id);
    d(pod.storage_status.device_status);
    d(pod.storage_status.progress_bar_value);
#undef d
    }

    out << "Payload:" << std::endl;
    out << pod.payload.dissect();
    return out.str();
  }
};
}
}

#endif
