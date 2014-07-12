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

#include "totpslot.h"
#include "string.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/


/*******************************************************************************

  TOTPSlot

  Constructor TOTPSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

TOTPSlot::TOTPSlot()
{
    isProgrammed=false;
    memset(slotName,0,sizeof(slotName));
    memset(secret,0,sizeof(secret));
    memset(tokenID,0,sizeof(tokenID));
    config=0;
    slotNumber=0;
    interval=30;
}

/*******************************************************************************

  TOTPSlot

  Constructor TOTPSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

TOTPSlot::TOTPSlot(uint8_t slotNumber, uint8_t slotName[], uint8_t secret[], uint8_t config)
{
    this->slotNumber=slotNumber;
    memcpy(this->slotName,slotName,15);
    memcpy(this->secret,secret,20);
    this->config=config;
}

