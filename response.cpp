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
#include <stdio.h>

#include "response.h"
#include "string.h"

#include "stick20responsedialog.h"


/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

  Response

  Constructor Response

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

Response::Response()
{
}

/*******************************************************************************

  DebugResponse

  Changes
  Date      Author          Info
  10.03.14  RB              Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

void Response::DebugResponse()
{
    char text[1000];
    char text1[1000];
    int i;
    static int Counter = 0;

    sprintf(text,"%6d :getResponse: ",Counter);
    Counter++;
    DebugAppendText (text);
    for (i=0;i<=64;i++)
    {
        sprintf(text,"%02x ",(unsigned char)reportBuffer[i]);
        DebugAppendText (text);
    }
    sprintf(text,"\n");
    DebugAppendText (text);

// Check device status =     deviceStatus = reportBuffer[1]
    switch (deviceStatus)
    {
        case CMD_STATUS_OK :
            DebugAppendText ((char *)"         Device status       : CMD_STATUS_OK\n");
            break;
        case CMD_STATUS_WRONG_CRC :
            DebugAppendText ((char *)"         Device status       : CMD_STATUS_WRONG_CRC\n");
            break;
        case CMD_STATUS_WRONG_SLOT :
            DebugAppendText ((char *)"         Device status       : CMD_STATUS_WRONG_SLOT\n");
            break;
        case CMD_STATUS_SLOT_NOT_PROGRAMMED :
            DebugAppendText ((char *)"         Device status       : CMD_STATUS_SLOT_NOT_PROGRAMMED\n");
            break;
        case CMD_STATUS_WRONG_PASSWORD :
            DebugAppendText ((char *)"         Device status       : CMD_STATUS_WRONG_PASSWORD\n");
            break;
        case CMD_STATUS_NOT_AUTHORIZED :
            DebugAppendText ((char *)"         Device status       : CMD_STATUS_NOT_AUTHORIZED\n");
            break;
        default:
            DebugAppendText ((char *)"         Device status       : Unknown\n");
            break;
    }


// Check last command =     lastCommandType   = reportBuffer[2]
    switch (lastCommandType)
    {
        case CMD_GET_STATUS :
            DebugAppendText ((char *)"         Last command        : CMD_GET_STATUS\n");
            break;
        case CMD_WRITE_TO_SLOT :
            DebugAppendText ((char *)"         Last command        : CMD_WRITE_TO_SLOT\n");
            break;
        case CMD_READ_SLOT_NAME :
            strncpy (text1,data,15);
            text1[15] = 0;
            sprintf(text,"         Last command        : CMD_READ_SLOT_NAME -%s-\n",text1);
            DebugAppendText (text);
            break;
        case CMD_READ_SLOT :
            DebugAppendText ((char *)"         Last command        : CMD_READ_SLOT\n");
            break;
        case CMD_GET_CODE :
            DebugAppendText ((char *)"         Last command        : CMD_GET_CODE\n");
            break;
        case CMD_WRITE_CONFIG :
            DebugAppendText ((char *)"         Last command        : CMD_WRITE_CONFIG\n");
            break;
        case CMD_ERASE_SLOT :
            DebugAppendText ((char *)"         Last command        : CMD_ERASE_SLOT\n");
            break;
        case CMD_FIRST_AUTHENTICATE :
            DebugAppendText ((char *)"         Last command        : CMD_FIRST_AUTHENTICATE\n");
            break;
        case CMD_AUTHORIZE :
            DebugAppendText ((char *)"         Last command        : CMD_AUTHORIZE\n");
            break;
        case CMD_GET_PASSWORD_RETRY_COUNT :
            DebugAppendText ((char *)"         Last command        : CMD_GET_PASSWORD_RETRY_COUNT\n");
            break;
        case CMD_CLEAR_WARNING :
            DebugAppendText ((char *)"         Last command        : CMD_CLEAR_WARNING\n");
            break;
        case CMD_GET_PW_SAFE_SLOT_STATUS :
            DebugAppendText ((char *)"         Last command        : CMD_GET_PW_SAFE_SLOT_STATUS\n");
            break;
        case CMD_GET_PW_SAFE_SLOT_NAME :
            DebugAppendText ((char *)"         Last command        : CMD_GET_PW_SAFE_SLOT_NAME\n");
            break;
        case CMD_GET_PW_SAFE_SLOT_PASSWORD :
            DebugAppendText ((char *)"         Last command        : CMD_GET_PW_SAFE_SLOT_PASSWORD\n");
            break;

        case CMD_GET_PW_SAFE_SLOT_LOGINNAME :
            DebugAppendText ((char *)"         Last command        : CMD_GET_PW_SAFE_SLOT_LOGINNAME\n");
            break;
        case CMD_SET_PW_SAFE_SLOT_DATA_1 :
            DebugAppendText ((char *)"         Last command        : CMD_SET_PW_SAFE_SLOT_DATA_1\n");
            break;
        case CMD_SET_PW_SAFE_SLOT_DATA_2 :
            DebugAppendText ((char *)"         Last command        : CMD_SET_PW_SAFE_SLOT_DATA_2\n");
            break;
        case CMD_PW_SAFE_ERASE_SLOT :
            DebugAppendText ((char *)"         Last command        : CMD_PW_SAFE_ERASE_SLOT\n");
            break;

        default:
            DebugAppendText ((char *)"         Last command        : Unknown\n");
            break;
    }




// Check last command status =     lastCommandStatus = reportBuffer[7]
    switch (lastCommandStatus)
    {
        case CMD_STATUS_OK :
            DebugAppendText ((char *)"         Last command status : CMD_STATUS_OK\n");
            break;
        case CMD_STATUS_WRONG_CRC :
            DebugAppendText ((char *)"         Last command status : CMD_STATUS_WRONG_CRC\n");
            break;
        case CMD_STATUS_WRONG_SLOT :
            DebugAppendText ((char *)"         Last command status : CMD_STATUS_WRONG_SLOT\n");
            break;
        case CMD_STATUS_SLOT_NOT_PROGRAMMED :
            DebugAppendText ((char *)"         Last command status : CMD_STATUS_SLOT_NOT_PROGRAMMED\n");
            break;
        case CMD_STATUS_WRONG_PASSWORD :
            DebugAppendText ((char *)"         Last command status : CMD_STATUS_WRONG_PASSWORD\n");
            break;
        case CMD_STATUS_NOT_AUTHORIZED :
            DebugAppendText ((char *)"         Last command status : CMD_STATUS_NOT_AUTHORIZED\n");
            break;
        default:
            DebugAppendText ((char *)"         Last command status : Unknown\n");
            break;
    }

//    lastCommandCRC     = ((uint32_t *)(reportBuffer+3))[0];
//    responseCRC       = ((uint32_t *)(reportBuffer+61))[0];



}

/*******************************************************************************

  getResponse

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Response::getResponse(Device *device)
{
     int res;

     memset(reportBuffer,0,sizeof(reportBuffer));

     if (NULL == device->dev_hid_handle)
     {
         return -1;
     }

     res = hid_get_feature_report(device->dev_hid_handle, reportBuffer, sizeof(reportBuffer));

//     qDebug() << "get report size:" << res;
    if (res!=-1)
    {
        deviceStatus      = reportBuffer[1];
        lastCommandType   = reportBuffer[2];
        lastCommandCRC    = ((uint32_t *)(reportBuffer+3))[0];
        lastCommandStatus = reportBuffer[7];
        responseCRC       = ((uint32_t *)(reportBuffer+61))[0];

        memcpy(data,reportBuffer+8,PAYLOAD_SIZE);

        // Copy Stick 2.0 status vom HID response data
        memcpy ((void*)&HID_Stick20Status_st,reportBuffer+1+OUTPUT_CMD_RESULT_STICK20_STATUS_START,sizeof (HID_Stick20Status_st));

        DebugResponse ();

        return 0;
    }
    else
    {
        return -1;
    }
 }



