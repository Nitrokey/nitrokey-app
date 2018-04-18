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


#ifndef MISC_H
#define MISC_H
#include <stdio.h>
#include <string>
#include <vector>
#include <string.h>
#include "log.h"
#include "LibraryException.h"
#include <sstream>
#include <iomanip>


namespace nitrokey {
namespace misc {

    template<typename T>
    std::string toHex(T value){
      using namespace std;
      std::ostringstream oss;
      oss << std::hex << std::setw(sizeof(value)*2) << std::setfill('0') << value;
      return oss.str();
    }

  /**
   * Copies string from pointer to fixed size C-style array. Src needs to be a valid C-string - eg. ended with '\0'.
   * Throws when source is bigger than destination.
   * @tparam T type of destination array
   * @param dest fixed size destination array
   * @param src pointer to source c-style valid string
   */
    template <typename T>
    void strcpyT(T& dest, const char* src){

        if (src == nullptr)
//            throw EmptySourceStringException(slot_number);
            return;
        const size_t s_dest = sizeof dest;
        LOG(std::string("strcpyT sizes dest src ")
                                       +std::to_string(s_dest)+ " "
                                       +std::to_string(strlen(src))+ " "
            ,nitrokey::log::Loglevel::DEBUG_L2);
        if (strlen(src) > s_dest){
            throw TooLongStringException(strlen(src), s_dest, src);
        }
        strncpy((char*) &dest, src, s_dest);
    }

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)  
    template <typename T>
typename T::CommandPayload get_payload(){
    //Create, initialize and return by value command payload
    typename T::CommandPayload st;
    bzero(&st, sizeof(st));
    return st;
}

    template<typename CMDTYPE, typename Tdev>
    void execute_password_command(Tdev &stick, const char *password) {
        auto p = get_payload<CMDTYPE>();
        p.set_defaults();
        strcpyT(p.password, password);
        CMDTYPE::CommandTransaction::run(stick, p);
    }

    std::string hexdump(const uint8_t *p, size_t size, bool print_header=true, bool print_ascii=true,
        bool print_empty=true);
    uint32_t stm_crc32(const uint8_t *data, size_t size);
    std::vector<uint8_t> hex_string_to_byte(const char* hexString);
}
}

#endif
