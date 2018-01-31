/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 * Copyright (c) 2012-2018 Nitrokey UG
 *
 * This file is part of Nitrokey App.
 *
 * Nitrokey App is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey App is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */


#ifndef HOTPSLOT_H
#define HOTPSLOT_H

#include <cstdint>
#include <vector>
#include <string>

static const unsigned int SECRET_LENGTH = 40;
static const unsigned int SECRET_LENGTH_BASE32 = SECRET_LENGTH / 10 * 16;
static const unsigned int SECRET_LENGTH_HEX = SECRET_LENGTH * 2;

std::vector<uint8_t> decodeBase32Secret(const std::string secret, const bool debug_mode = false);


class OTPSlot {
public:
    enum OTPType{
        UNKNOWN, HOTP, TOTP
    };

    OTPSlot();
    ~OTPSlot(){
      volatile char* p;
      p = slotName;
      for (uint64_t i = 0; i < sizeof(slotName); ++i) {
        p[i] = 0;
      }
      p = secret;
      for (uint64_t i = 0; i < sizeof(secret); ++i) {
        p[i] = 0;
      }
      slotNumber = 0;
      type = OTPType::UNKNOWN;
    }

    OTPType type;
    uint8_t slotNumber;
    char slotName[15+1] = {};
    char secret[SECRET_LENGTH_HEX+1] = {};
    union {
        uint8_t counter[8];
        uint64_t interval;
    };
    union{
    uint8_t config;
        struct {
            bool useEightDigits :1;
            bool useEnter :1;
            bool useTokenID :1;
        } config_st;
    };
    char tokenID[13+1] = {};
    bool isProgrammed;
};

#endif // HOTPSLOT_H
