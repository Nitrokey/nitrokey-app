/*
* Author: Copyright (C) Andrzej Surowiec 2012
*						Parts Rudolf Boeddeker  Date: 2013-08-13
*
* This file is part of GPF Crypto Stick.
*
* GPF Crypto Stick is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* GPF Crypto Stick is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GPF Crypto Stick. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COMMAND_H
#define COMMAND_H

#include "inttypes.h"

#define COMMAND_SIZE 59

class Command
{
public:
    Command(uint8_t commandType, uint8_t *data,uint8_t len);
    uint8_t commandType;
    uint8_t data[COMMAND_SIZE];
    uint8_t len;
    uint32_t crc;

    void generateCRC();

};


#endif // COMMAND_H
