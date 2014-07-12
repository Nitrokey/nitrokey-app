/*
* Author: Copyright (C) Andrzej Surowiec 2012
*
*
* This file is part of GPF Crypto Stick.
*						Parts Rudolf Boeddeker  Date: 2013-08-13
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

#ifndef RESPONSE_H
#define RESPONSE_H

#include "stick20hid.h"
#include "device.h"

class Response
{
public:
    Response();
    void DebugResponse();

    uint8_t deviceStatus;
    uint8_t lastCommandType;
    uint32_t lastCommandCRC;
    uint8_t lastCommandStatus;
    char data[PAYLOAD_SIZE];
    uint32_t responseCRC;
    int getResponse(Device *device);
    uint8_t reportBuffer[REPORT_SIZE+1];


    HID_Stick20Status_est HID_Stick20Status_st;
};

#endif // RESPONSE_H
