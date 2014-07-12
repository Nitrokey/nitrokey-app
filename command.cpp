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

#include "command.h"
#include "string.h"
#include "crc32.h"
#include "device.h"

/*******************************************************************************

  Command

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

Command::Command(uint8_t commandType, uint8_t *data, uint8_t len)
{
    this->commandType = commandType;

    memset(this->data,0,COMMAND_SIZE);

    if (COMMAND_SIZE < len)
    {
        len = COMMAND_SIZE;
    }

    if ((0 != len) && (NULL != data))
    {
        memcpy(this->data,data,len);
    }
}

/*******************************************************************************

  generateCRC

  Todo: Check length of COMMAND_SIZE (2+59 = 61 not 60 !!!)

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void Command::generateCRC()
{
    int i;
    uint8_t report[REPORT_SIZE+1];
    uint32_t crc=0xffffffff;

    memset(report,0,sizeof(report));

    report[1]=this->commandType;

    memcpy(report+2,this->data,COMMAND_SIZE); // (2+59 = 61 not 60 !!!)

    for (i=0;i<15;i++)
    {
        crc=Crc32(crc,((uint32_t *)(report+1))[i]);
    }

    ((uint32_t *)(report+1))[15]=crc;

    this->crc=crc;
}
