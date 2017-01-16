/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *
 *
 * This file is part of Nitrokey.
 *
 * Nitrokey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HOTPSLOT_H
#define HOTPSLOT_H

#include <stdint.h>

static const int SECRET_LENGTH = 40;
static const int SECRET_LENGTH_BASE32 = SECRET_LENGTH / 10 * 16;
static const int SECRET_LENGTH_HEX = SECRET_LENGTH * 2;



class OTPSlot {
public:
    enum OTPType{
        UNKNOWN, HOTP, TOTP
    };

    OTPSlot();

    OTPType type;
    uint8_t slotNumber;
    char slotName[15];
    char secret[SECRET_LENGTH];
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
    char tokenID[13];
    bool isProgrammed;
};

#endif // HOTPSLOT_H
