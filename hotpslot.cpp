/*
* Author: Copyright (C) Andrzej Surowiec 2012
*
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

#include "hotpslot.h"
#include "string.h"


HOTPSlot::HOTPSlot(){
    isProgrammed=false;
    memset(slotName,0,sizeof(slotName));
    memset(secret,0,sizeof(secret));
    memset(counter,0,sizeof(counter));
    memset(tokenID,0,sizeof(tokenID));
    config=0;
    slotNumber=0;

}

HOTPSlot::HOTPSlot(uint8_t slotNumber,uint8_t slotName[20],uint8_t secret[20],uint8_t counter[8], uint8_t config)
{
    this->slotNumber=slotNumber;
    memcpy(this->slotName,slotName,15);
    memcpy(this->secret,secret,20);
    memcpy(this->counter,counter,8);
    this->config=config;

}




