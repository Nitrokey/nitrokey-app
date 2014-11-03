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

#include <QDebug>
#include <QDateTime>

#include "response.h"
#include "string.h"
#include "crc32.h"
#include "sleep.h"
#include "device.h"

/*******************************************************************************

 External declarations

*******************************************************************************/


/*******************************************************************************

 Local defines

*******************************************************************************/

//#define LOCAL_DEBUG                               // activate for debugging

/*******************************************************************************

  Device

  Constructor Device

  Changes
  Date      Author          Info
  25.03.14  RB              Dynamic slot counts

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

Device::Device(int vid, int pid,int vidStick20, int pidStick20,int vidStick20UpdateMode, int pidStick20UpdateMode)
{
    int i;

    LastStickError = OUTPUT_CMD_STICK20_STATUS_OK;

    dev_hid_handle=NULL;
    isConnected=false;

    this->vid=vid;
    this->pid=pid;


    validPassword    =false;
    passwordSet      =false;

    validUserPassword = false;

    memset(password,0,50);
    memset(userPassword,0,50);

// Vars for password safe
    passwordSafeUnlocked = FALSE;
    passwordSafeAvailable = TRUE;
    memset(passwordSafeStatusDisplayed,0,PWS_SLOT_COUNT);

    for (i=0;i<PWS_SLOT_COUNT;i++)
    {
        passwordSafeSlotNames[i][0] = 0;
    }


    //handle = hid_open(vid,pid, NULL);

    HOTP_SlotCount = HOTP_SLOT_COUNT;       // For stick 1.0
    TOTP_SlotCount = TOTP_SLOT_COUNT;

    for (i=0;i<HOTP_SLOT_COUNT_MAX;i++)
    {
        HOTPSlots[i]             = new HOTPSlot();
        HOTPSlots[i]->slotNumber = 0x10 + i;
    }

    for (i=0;i<TOTP_SLOT_COUNT_MAX;i++)
    {
        TOTPSlots[i]             = new TOTPSlot();
        TOTPSlots[i]->slotNumber = 0x20 + i;
    }

    newConnection=true;

// Init data for stick 20

   this->vidStick20             = vidStick20;
   this->pidStick20             = pidStick20;
   this->vidStick20UpdateMode   = vidStick20UpdateMode;
   this->pidStick20UpdateMode   = pidStick20UpdateMode;

//    this->vidStick20   = 0x046d;
//    this->pidStick20   = 0xc315;

    activStick20       = false;
    waitForAckStick20  = false;
    lastBlockNrStick20 = 0;


}

/*******************************************************************************

  checkConnection

  Check the presents of stick 1.0 or 2.0

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::checkConnection()
{
    uint8_t buf[65];
    buf[0]=0;
    int res;

    //handle = hid_open(vid,pid, NULL);

    if (!dev_hid_handle)
    {
        isConnected   = false;
        newConnection = true;
        return -1;
    }
    else{
        res = hid_get_feature_report (dev_hid_handle, buf, 65);
        if (res < 0){ // Check it twice to avoid connection problem
            res = hid_get_feature_report (dev_hid_handle, buf, 65);
            if (res < 0){
                isConnected   = false;
                newConnection = true;
                return -1;
            }
        }

        if (newConnection)
        {
            isConnected   = true;
            newConnection = false;
            passwordSet   = false;
            validPassword = false;
            validUserPassword = false;

            HOTP_SlotCount = HOTP_SLOT_COUNT;       // For stick 1.0
            TOTP_SlotCount = TOTP_SLOT_COUNT;

            passwordSafeUnlocked = FALSE;

            // stick 20 with no OTP
            if (true == activStick20)
            {
                HOTP_SlotCount = HOTP_SLOT_COUNT_MAX;
                TOTP_SlotCount = TOTP_SLOT_COUNT_MAX;
                passwordSafeUnlocked = FALSE;
            }
            initializeConfig();          // init data for OTP
            return 1;
        }

        return 0;
    }
}



/*******************************************************************************

  connect

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Device::connect()
{
    // Disable stick 20
    activStick20 = false;

    dev_hid_handle = hid_open(vid,pid, NULL);

    // Check for stick 20
    if (NULL == dev_hid_handle)
    {
        dev_hid_handle = hid_open(vidStick20,pidStick20, NULL);
        if (NULL != dev_hid_handle)
        {
            // Stick 20 found
            activStick20 = true;
        }
    }       
}

/*******************************************************************************

  sendCommand

  Send a command to the stick via the send feature report HID message

  Report size is 64 byte

  Command data size COMMAND_SIZE = 59 byte (should 58 ???)

  Byte  0       = 0
  Byte  1       = cmd type
  Byte  2-59    = payload       (To do check length)
  Byte 60-63    = CRC 32 from byte 0-59 = 15 long words


  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::sendCommand(Command *cmd)
{
    uint8_t report[REPORT_SIZE+1];
    int i;
    int err;

    memset(report,0,sizeof(report));
    report[1]=cmd->commandType;


    memcpy(report+2,cmd->data,COMMAND_SIZE);

    uint32_t crc=0xffffffff;
    for (i=0;i<15;i++){
        crc=Crc32(crc,((uint32_t *)(report+1))[i]);
    }
    ((uint32_t *)(report+1))[15]=crc;

    cmd->crc=crc;
/*
    {
         char text[1000];
         sprintf(text,"computed crc :%08x:\n",crc );
         DebugAppendText (text);
    }
*/
    err = hid_send_feature_report(dev_hid_handle, report, sizeof(report));

    {
            char text[1000];
            int i;
            static int Counter = 0;

            sprintf(text,"%6d :sendCommand0: ", Counter);
            Counter++;
            DebugAppendText (text);
            for (i=0;i<=64;i++)
            {
                sprintf(text,"%02x ",(unsigned char)report[i]);
                DebugAppendText (text);
            }
            sprintf(text,"\n");
            DebugAppendText (text);

     }

    return err;
}


int Device::sendCommandGetResponse(Command *cmd, Response *resp)
{
    uint8_t report[REPORT_SIZE+1];
    int i;
    int err;

    if (!isConnected)
        return ERR_NOT_CONNECTED;

    memset(report,0,sizeof(report));
    report[1]=cmd->commandType;


    memcpy(report+2,cmd->data,COMMAND_SIZE);

    uint32_t crc=0xffffffff;
    for (i=0;i<15;i++){
        crc=Crc32(crc,((uint32_t *)(report+1))[i]);
    }
    ((uint32_t *)(report+1))[15]=crc;

    cmd->crc=crc;

    err = hid_send_feature_report(dev_hid_handle, report, sizeof(report));

    {
            char text[1000];
            int i;
            static int Counter = 0;

            sprintf(text,"%6d :sendCommand1: ",Counter);
            Counter++;
            DebugAppendText (text);
            for (i=0;i<=64;i++)
            {
                sprintf(text,"%02x ",(unsigned char)report[i]);
                DebugAppendText (text);
            }
            sprintf(text,"\n");
            DebugAppendText (text);

     }

    if (err==-1)
        return ERR_SENDING;

    Sleep::msleep(100);

    //Response *resp=new Response();
    resp->getResponse(this);

    if (cmd->crc!=resp->lastCommandCRC)
        return ERR_WRONG_RESPONSE_CRC;

    if (resp->lastCommandStatus==CMD_STATUS_OK)
        return 0;



    return ERR_STATUS_NOT_OK;
}


/*******************************************************************************

  getSlotName

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::getSlotName(uint8_t slotNo){
     int res;
     uint8_t data[1];

     data[0]=slotNo;


     if (isConnected){
     Command *cmd=new Command(CMD_READ_SLOT_NAME,data,1);
     res=sendCommand(cmd);

     if (res==-1)
         return -1;
     else{  //sending the command was successful
         //return cmd->crc;
         Sleep::msleep(100);
         Response *resp=new Response();
         resp->getResponse(this);

//         qDebug() << cmd->crc;
//         qDebug() << resp->lastCommandCRC;

         if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
             if (resp->lastCommandStatus==CMD_STATUS_OK)
             {
                 if ((slotNo >= 0x10) && (slotNo < 0x10 + HOTP_SlotCount)){
                    memcpy(HOTPSlots[slotNo&0x0F]->slotName,resp->data,15);
                    HOTPSlots[slotNo&0x0F]->isProgrammed=true;
                 }
                 else if ((slotNo >= 0x20) && (slotNo < 0x20 + TOTP_SlotCount)){
                     memcpy(TOTPSlots[slotNo&0x0F]->slotName,resp->data,15);
                     TOTPSlots[slotNo&0x0F]->isProgrammed=true;
                 }

             }
             else if (resp->lastCommandStatus==CMD_STATUS_SLOT_NOT_PROGRAMMED)
             {
                 if ((slotNo >= 0x10) && (slotNo < 0x10 + HOTP_SlotCount))
                 {
                    HOTPSlots[slotNo&0x0F]->isProgrammed=false;
                    HOTPSlots[slotNo&0x0F]->slotName[0] = 0;
                 }
                 else if ((slotNo >= 0x20) && (slotNo < 0x20 + TOTP_SlotCount))
                 {
                    TOTPSlots[slotNo&0x0F]->isProgrammed=false;
                    TOTPSlots[slotNo&0x0F]->slotName[0] = 0;
                 }
             }

         }
/*
         {

             QMessageBox message;
             QString str;
             QByteArray *data =new QByteArray((char*)resp->reportBuffer,REPORT_SIZE+1);

 //            str.append(QString::number(testResponse->lastCommandCRC,16));
             str.append(QString(data->toHex()));

             message.setText(str);
             message.exec();

             str.clear();
         }
*/
         return 0;
     }

    }

     return -1;
}

/*******************************************************************************

  eraseSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::eraseSlot(uint8_t slotNo)
{
    int res;
    uint8_t data[1];

    data[0]=slotNo;



    if (isConnected){
    Command *cmd=new Command(CMD_ERASE_SLOT,data,1);
    authorize(cmd);
    res=sendCommand(cmd);

    if (res==-1)
        return -1;
    else{  //sending the command was successful
        //return cmd->crc;
        Sleep::msleep(100);
        Response *resp=new Response();
        resp->getResponse(this);

//        qDebug() << cmd->crc;
//        qDebug() << resp->lastCommandCRC;

        }

        return 0;
    }

    return -1;

}

/*******************************************************************************

  setTime

  reset  =  0   Get time
            1   Set time

  Reviews
  Date      Reviewer        Info
  27.07.14  SN              First review

*******************************************************************************/

int Device::setTime(int reset){
     int res;
     uint8_t data[30];
     uint64_t time = QDateTime::currentDateTime().toTime_t();

     memset(data,0,30);
     data[0] = reset;
     memcpy(data+1,&time,8);

     if (isConnected){
     Command *cmd=new Command(CMD_SET_TIME,data,9);
     res=sendCommand(cmd);

     if (res==-1)
         return -1;
     else{  //sending the command was successful
         Sleep::msleep(100);
         Response *resp=new Response();
         resp->getResponse(this);

         if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
             if (resp->lastCommandStatus==CMD_STATUS_OK)
             {
             }
             else if (resp->lastCommandStatus==CMD_STATUS_TIMESTAMP_WARNING)
             {
                 return -2;
             }

         }
         return 0;
     }

    }

     return -1;
}

/*******************************************************************************

  writeToHOTPSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::writeToHOTPSlot(HOTPSlot *slot)
{
//    qDebug() << "preparing to send";
//    qDebug() << slot->slotNumber;
//    qDebug() << QString((char *)slot->slotName);

    if ((slot->slotNumber >= 0x10) && (slot->slotNumber < 0x10 + HOTP_SlotCount)){
        int res;
        uint8_t data[COMMAND_SIZE];
        memset(data,0,COMMAND_SIZE);
//        qDebug() << "prepared data array";

        data[0]=slot->slotNumber;
        memcpy(data+1,slot->slotName,15);
        memcpy(data+16,slot->secret,20);
        data[36]=slot->config;
        memcpy(data+37,slot->tokenID,13);
        memcpy(data+50,slot->counter,8);

//        qDebug() << "copied data to array";

        if (isConnected)
        {
            Command *cmd=new Command(CMD_WRITE_TO_SLOT,data,COMMAND_SIZE);
    //        qDebug() << "sending";
            authorize(cmd);
            res=sendCommand(cmd);
    //        qDebug() << "sent";

            if (res==-1)
                return -1;
            else{  //sending the command was successful
                //return cmd->crc;
                Sleep::msleep(100);
                Response *resp=new Response();
                resp->getResponse(this);

                 if (cmd->crc==resp->lastCommandCRC&&resp->lastCommandStatus==CMD_STATUS_OK){
    //                 qDebug() << "sent sucessfully!";
                     return 0;

                 } else if (cmd->crc==resp->lastCommandCRC&&resp->lastCommandStatus==CMD_STATUS_NO_NAME_ERROR){
                     return -3;
                 }

            }

            return -2;
        }


    }
    return -1;
}

/*******************************************************************************

  writeToTOTPSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::writeToTOTPSlot(TOTPSlot *slot)
{
    if ((slot->slotNumber >= 0x20) && (slot->slotNumber < 0x20 + TOTP_SlotCount))
    {
        int res;
        uint8_t data[COMMAND_SIZE];
        memset(data,0,COMMAND_SIZE);
//        qDebug() << "prepared data array";

        data[0]=slot->slotNumber;
        memcpy(data+1,slot->slotName,15);
        memcpy(data+16,slot->secret,20);
        data[36]=slot->config;
        memcpy(data+37,slot->tokenID,13);
	memcpy(data+50,&(slot->interval),2);

//        qDebug() << "copied data to array";

        if (isConnected){
        Command *cmd=new Command(CMD_WRITE_TO_SLOT,data,COMMAND_SIZE);
//        qDebug() << "sending";
        authorize(cmd);
        res=sendCommand(cmd);
//        qDebug() << "sent";

        if (res==-1)
            return -1;
        else{  //sending the command was successful
            //return cmd->crc;
            Sleep::msleep(100);
            Response *resp=new Response();
            resp->getResponse(this);

             if (cmd->crc==resp->lastCommandCRC&&resp->lastCommandStatus==CMD_STATUS_OK){
//                 qDebug() << "sent sucessfully!";
                 return 0;

             } else if (cmd->crc==resp->lastCommandCRC&&resp->lastCommandStatus==CMD_STATUS_NO_NAME_ERROR){
                 return -3;
             }

        }

        return -2;
        }


    }
    return -1;


}

/*******************************************************************************

  getCode

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::getCode(uint8_t slotNo, uint64_t challenge,uint64_t lastTOTPTime,uint8_t  lastInterval,uint8_t result[18])
{

//    qDebug() << "getting code" << slotNo;
    int res;
    uint8_t data[30];

    data[0]=slotNo;

    memcpy(data+ 1,&challenge,8);

    memcpy(data+ 9,&lastTOTPTime,8);     // Time of challenge: Warning: it's better to tranfer time and interval, to avoid attacks with wrong timestamps
    memcpy(data+17,&lastInterval,1);

    if (isConnected){
//       qDebug() << "sending command";

    Command *cmd=new Command(CMD_GET_CODE,data,18);
    userAuthorize(cmd);
    res=sendCommand(cmd);

    if (res==-1)
        return -1;
    else{  //sending the command was successful
        //return cmd->crc;
//         qDebug() << "command sent";
        Sleep::msleep(100);
        Response *resp=new Response();
        resp->getResponse(this);


        if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
            if (resp->lastCommandStatus==CMD_STATUS_OK){
                memcpy(result,resp->data,18);

            }

        }

        return 0;
    }

   }

    return -1;

}


int Device::getHOTP(uint8_t slotNo)
{

//    qDebug() << "getting code" << slotNo;
    int res;
    uint8_t data[9];

    data[0]=slotNo;

    //memcpy(data+1,&challenge,8);


    Command *cmd=new Command(CMD_GET_CODE,data,9);
    Response *resp=new Response();
    res=sendCommandGetResponse(cmd,resp);

        //    if (res==0)
        //        memcpy(result,resp->data,18);

    return res;

}

/*******************************************************************************

  readSlot

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::readSlot(uint8_t slotNo)
{
    int res;
    uint8_t data[1];

    data[0]=slotNo;


    if (isConnected){
    Command *cmd=new Command(CMD_READ_SLOT,data,1);
    res=sendCommand(cmd);

    if (res==-1)
        return -1;
    else{  //sending the command was successful
        //return cmd->crc;
        Sleep::msleep(100);
        Response *resp=new Response();
        resp->getResponse(this);

//         qDebug() << cmd->crc;
//         qDebug() << resp->lastCommandCRC;

        if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
            if (resp->lastCommandStatus==CMD_STATUS_OK)
            {
                if ((slotNo >= 0x10) && (slotNo < 0x10 + HOTP_SlotCount))
                {
                   memcpy(HOTPSlots[slotNo&0x0F]->slotName,resp->data,15);
                   HOTPSlots[slotNo&0x0F]->config = resp->data[15];
                   memcpy(HOTPSlots[slotNo&0x0F]->tokenID,resp->data+16,13);
                   memcpy(HOTPSlots[slotNo&0x0F]->counter,resp->data+29,8);
                   HOTPSlots[slotNo&0x0F]->isProgrammed=true;
/*
qDebug() << "Get HOTP counter slot " << (slotNo&0x0F);
qDebug() << QString ((char*)HOTPSlots[slotNo&0x0F]->counter);
*/
                }
                else if ((slotNo >= 0x20) && (slotNo < 0x20 + TOTP_SlotCount))
                {
                    memcpy(TOTPSlots[slotNo&0x0F]->slotName,resp->data,15);
                    TOTPSlots[slotNo&0x0F]->config = resp->data[15];
                    memcpy(TOTPSlots[slotNo&0x0F]->tokenID,resp->data+16,13);
                    memcpy(&(TOTPSlots[slotNo&0x0F]->interval),resp->data+29,2);
                    TOTPSlots[slotNo&0x0F]->isProgrammed=true;
                }

            }
            else if (resp->lastCommandStatus==CMD_STATUS_SLOT_NOT_PROGRAMMED)
            {
                if ((slotNo >= 0x10) && (slotNo < 0x10 + HOTP_SlotCount))
                {
                   HOTPSlots[slotNo&0x0F]->isProgrammed=false;
                   HOTPSlots[slotNo&0x0F]->slotName[0] = 0;
                }
                else if ((slotNo >= 0x20) && (slotNo < 0x20 + TOTP_SlotCount))
                {
                   TOTPSlots[slotNo&0x0F]->isProgrammed=false;
                   TOTPSlots[slotNo&0x0F]->slotName[0] = 0;
                }
            }

        }
/*
        {

            QMessageBox message;
            QString str;
            QByteArray *data =new QByteArray((char*)resp->reportBuffer,REPORT_SIZE+1);

//            str.append(QString::number(testResponse->lastCommandCRC,16));
            str.append(QString(data->toHex()));

            message.setText(str);
            message.exec();

            str.clear();
        }
*/
        return 0;
    }

   }

    return -1;
}
/*******************************************************************************

  initializeConfig

  Changes
  Date      Author          Info
  25.03.14  RB              Slot count by define

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Device::initializeConfig()
{
    int i;
    unsigned int currentTime;

    for (i=0;i<HOTP_SlotCount;i++)
    {
        readSlot(0x10 + i);
    }

    for (i=0;i<TOTP_SlotCount;i++)
    {
        readSlot(0x20 + i);
    }

    if (true == activStick20)
    {
        currentTime = QDateTime::currentDateTime().toTime_t();
        stick20SendStartup ((unsigned long long)currentTime);
    }
}

/*******************************************************************************

  getSlotConfigs

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Device::getSlotConfigs()
{
 /* Removed from firmware
    readSlot(0x10);
    readSlot(0x11);
    readSlot(0x20);
    readSlot(0x21);
    readSlot(0x22);
    readSlot(0x23);*/
}
/*******************************************************************************

  getStatus

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::getStatus()
{
    int res;
    uint8_t data[1];


    if (isConnected){
    Command *cmd=new Command(CMD_GET_STATUS,data,0);
    res=sendCommand(cmd);

    if (res==-1)
        return -1;
    else{  //sending the command was successful
        Sleep::msleep(100);
        Response *resp=new Response();
        resp->getResponse(this);

        if (cmd->crc==resp->lastCommandCRC){
            memcpy(firmwareVersion,resp->data,2);
            memcpy(cardSerial,resp->data+2,4);
            memcpy(generalConfig,resp->data+6,3);
            memcpy(otpPasswordConfig,resp->data+9,2);
        }
    }
    return 0;
    }
    return -2;
}

/*******************************************************************************

  getPasswordRetryCount

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordRetryCount()
{
    int res;
    uint8_t data[1];


    if (isConnected){
    Command *cmd=new Command(CMD_GET_PASSWORD_RETRY_COUNT,data,0);
    res=sendCommand(cmd);

    if (res==-1)
        return ERR_SENDING;
    else{  //sending the command was successful
        Sleep::msleep(1000);
        Response *resp=new Response();
        resp->getResponse(this);

        if (cmd->crc==resp->lastCommandCRC)
        {
            passwordRetryCount=resp->data[0];
            HID_Stick20Configuration_st.AdminPwRetryCount = passwordRetryCount;
        }
        else
            return ERR_WRONG_RESPONSE_CRC;
    }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getUserPasswordRetryCount

  Changes
  Date      Reviewer        Info
  10.08.14  SN              Function created

*******************************************************************************/

int Device::getUserPasswordRetryCount()
{
    int res;
    uint8_t data[1];


    if (isConnected){
    Command *cmd=new Command(CMD_GET_USER_PASSWORD_RETRY_COUNT,data,0);
    res=sendCommand(cmd);

    if (res==-1)
        return ERR_SENDING;
    else{  //sending the command was successful
        Sleep::msleep(1000);
        Response *resp=new Response();
        resp->getResponse(this);

        if (cmd->crc==resp->lastCommandCRC)
        {
            userPasswordRetryCount=resp->data[0];
            HID_Stick20Configuration_st.UserPwRetryCount = userPasswordRetryCount;
        }
        else
            return ERR_WRONG_RESPONSE_CRC;
    }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getPasswordSafeSlotStatus

  Changes
  Date      Author        Info
  29.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordSafeSlotStatus ()
{
    int res;
    int i;
    uint8_t data[1];

// Clear entrys
    memset (passwordSafeStatus,0,PWS_SLOT_COUNT);

    if (isConnected)
    {
        Command *cmd=new Command(CMD_GET_PW_SAFE_SLOT_STATUS,data,0);
        res=sendCommand(cmd);

        if (res==-1)
        {
            return ERR_SENDING;
        }
        else
        {                       //sending the command was successful
            Sleep::msleep(500);

            Response *resp=new Response();
            resp->getResponse(this);
            LastStickError = resp->deviceStatus;

            if (cmd->crc == resp->lastCommandCRC)
            {
                if (resp->lastCommandStatus == CMD_STATUS_OK)
                {
                    memcpy (passwordSafeStatus,&resp->data[0],PWS_SLOT_COUNT);
/*
{
     char text[1000];
     DebugAppendText ("PW_SAFE_SLOT_STATUS\n");
     for (i=0;i<PWS_SLOT_COUNT;i++)
     {
         sprintf(text,"  %2d : %3d\n",i,passwordSafeStatus[i]);
         DebugAppendText (text);
     }
}
*/

                    return (ERR_NO_ERROR);
                }
                else
                {
                    return (ERR_STATUS_NOT_OK);
                }

            }
            else
                return ERR_WRONG_RESPONSE_CRC;
        }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getPasswordSafeSlotName

  Changes
  Date      Author        Info
  29.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordSafeSlotName (int Slot)
{
    int res;
    uint8_t data[1];

    data[0] = Slot;

    if (isConnected)
    {
        Command *cmd=new Command(CMD_GET_PW_SAFE_SLOT_NAME,data,1);
        res=sendCommand(cmd);

        if (res==-1)
            return ERR_SENDING;
        else{  //sending the command was successful
            Sleep::msleep(200);
            Response *resp=new Response();
            resp->getResponse(this);

            if (cmd->crc==resp->lastCommandCRC)
            {
                memcpy (passwordSafeSlotName,&resp->data[0],PWS_SLOTNAME_LENGTH);
                passwordSafeSlotName[PWS_SLOTNAME_LENGTH] = 0;
                if (resp->lastCommandStatus == CMD_STATUS_OK)
                {
                    return (ERR_NO_ERROR);
                }
                else
                {
                    return (ERR_STATUS_NOT_OK);
                }
/*
{
    char text[1000];
    sprintf(text,"getPasswordSafeSlotName: Slot %2d : -%s-\n",Slot,passwordSafeSlotName);
    DebugAppendText (text);
}
*/
            }
            else
                return ERR_WRONG_RESPONSE_CRC;
        }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getPasswordSafeSlotPassword

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordSafeSlotPassword (int Slot)
{
    int res;
    uint8_t data[1];

    data[0] = Slot;

    if (isConnected){
    Command *cmd=new Command(CMD_GET_PW_SAFE_SLOT_PASSWORD,data,1);
    res=sendCommand(cmd);

    if (res==-1)
        return ERR_SENDING;
    else{  //sending the command was successful
        Sleep::msleep(200);
        Response *resp=new Response();
        resp->getResponse(this);

        if (cmd->crc==resp->lastCommandCRC)
        {
            memcpy (passwordSafePassword,&resp->data[0],PWS_PASSWORD_LENGTH);
            passwordSafePassword[PWS_PASSWORD_LENGTH] = 0;

            if (resp->lastCommandStatus == CMD_STATUS_OK)
            {
                return (ERR_NO_ERROR);
            }
            else
            {
                return (ERR_STATUS_NOT_OK);
            }
/*
{
    char text[1000];
    sprintf(text,"getPasswordSafeSlotPassword: Slot %2d : -%s-\n",Slot,passwordSafePassword);
    DebugAppendText (text);
}
*/
        }
        else
            return ERR_WRONG_RESPONSE_CRC;
    }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  getPasswordSafeSlotLoginName

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::getPasswordSafeSlotLoginName (int Slot)
{
    int res;
    uint8_t data[1];

    data[0] = Slot;

    if (isConnected){
    Command *cmd=new Command(CMD_GET_PW_SAFE_SLOT_LOGINNAME,data,1);
    res=sendCommand(cmd);

    if (res==-1)
        return ERR_SENDING;
    else{  //sending the command was successful
        Sleep::msleep(200);
        Response *resp=new Response();
        resp->getResponse(this);

        if (cmd->crc==resp->lastCommandCRC)
        {
            memcpy (passwordSafeLoginName,&resp->data[0],PWS_LOGINNAME_LENGTH);
            passwordSafeLoginName[PWS_LOGINNAME_LENGTH] = 0;
            if (resp->lastCommandStatus == CMD_STATUS_OK)
            {
                return (ERR_NO_ERROR);
            }
            else
            {
                return (ERR_STATUS_NOT_OK);
            }
/*
{
    char text[1000];
    sprintf(text,"getPasswordSafeSlotLoginName: Slot %2d : -%s-\n",Slot,passwordSafeLoginName);
    DebugAppendText (text);
}
*/
        }
        else
            return ERR_WRONG_RESPONSE_CRC;
    }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  setPasswordSafeSlotData_1

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::setPasswordSafeSlotData_1 (int Slot,uint8_t *Name,uint8_t *Password)
{
    int res;
    uint8_t data[1+PWS_SLOTNAME_LENGTH+PWS_PASSWORD_LENGTH+1];

    data[0] = Slot;
    memcpy (&data[1],Name,PWS_SLOTNAME_LENGTH);
    memcpy (&data[1+PWS_SLOTNAME_LENGTH],Password,PWS_PASSWORD_LENGTH);

    if (isConnected)
    {
        Command *cmd = new Command(CMD_SET_PW_SAFE_SLOT_DATA_1,data,1+PWS_SLOTNAME_LENGTH+PWS_PASSWORD_LENGTH);
        res = sendCommand(cmd);

        if (res==-1)
            return ERR_SENDING;
        else
        {  //sending the command was successful
            Sleep::msleep(200);
            Response *resp=new Response();
            resp->getResponse(this);

            if (cmd->crc==resp->lastCommandCRC)
            {
                  return ERR_NO_ERROR;
//                passwordRetryCount=resp->data[0];
            }
            else
                return ERR_WRONG_RESPONSE_CRC;
        }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  setPasswordSafeSlotData_2

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::setPasswordSafeSlotData_2 (int Slot,uint8_t *LoginName)
{
    int res;
    uint8_t data[1+PWS_LOGINNAME_LENGTH+1];

    data[0] = Slot;
    memcpy (&data[1],LoginName,PWS_LOGINNAME_LENGTH);

    if (isConnected){
    Command *cmd=new Command(CMD_SET_PW_SAFE_SLOT_DATA_2,data,1+PWS_LOGINNAME_LENGTH);
    res=sendCommand(cmd);

    if (res==-1)
        return ERR_SENDING;
    else{  //sending the command was successful
        Sleep::msleep(400);
        Response *resp=new Response();
        resp->getResponse(this);

        if (cmd->crc == resp->lastCommandCRC)
        {
              return ERR_NO_ERROR;
//            passwordRetryCount=resp->data[0];
        }
        else
            return ERR_WRONG_RESPONSE_CRC;
    }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  passwordSafeEraseSlot

  Changes
  Date      Author        Info
  30.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::passwordSafeEraseSlot (int Slot)
{
    int res;
    uint8_t data[1];

    data[0] = Slot;

    if (isConnected)
    {
        Command *cmd=new Command(CMD_PW_SAFE_ERASE_SLOT,data,1);
        res=sendCommand(cmd);

        if (res==-1)
            return ERR_SENDING;
        else
        {  //sending the command was successful
            Sleep::msleep(500);
            Response *resp=new Response();
            resp->getResponse(this);

            if (cmd->crc==resp->lastCommandCRC)
            {
                return resp->lastCommandStatus;
                if (resp->lastCommandStatus == CMD_STATUS_OK)
                {
                    return (ERR_NO_ERROR);
                }
                else
                {
                    return (ERR_STATUS_NOT_OK);
                }
            }
            else
                return ERR_WRONG_RESPONSE_CRC;
        }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  passwordSafeEnable

  Changes
  Date      Author        Info
  01.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::passwordSafeEnable (char *password)
{
    int res;
    uint8_t data[50];

    strncpy ((char*)data,password,30);
    data[30+1] = 0;

    if (isConnected)
    {
        Command *cmd=new Command(CMD_PW_SAFE_ENABLE,data,strlen ((char*)data));
        res=sendCommand(cmd);

        if (res==-1)
            return ERR_SENDING;
        else
        {  //sending the command was successful
            Sleep::msleep(1500);
            Response *resp=new Response();
            resp->getResponse(this);

            if (cmd->crc==resp->lastCommandCRC)
            {

                if (resp->lastCommandStatus == CMD_STATUS_OK)
                {
                    passwordSafeUnlocked = TRUE;
                    HID_Stick20Configuration_st.UserPwRetryCount = 3;
                    return (ERR_NO_ERROR);
                }
                else
                {
                    if (0 < HID_Stick20Configuration_st.UserPwRetryCount)
                    {
                        HID_Stick20Configuration_st.UserPwRetryCount--;
                    }
                    return resp->lastCommandStatus;
                    // return (ERR_STATUS_NOT_OK);
                }
            }
            else
                return ERR_WRONG_RESPONSE_CRC;
        }
    }
    return ERR_NOT_CONNECTED;
}


/*******************************************************************************

  passwordSafeInitKey

  Changes
  Date      Author        Info
  01.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::passwordSafeInitKey (void)
{
    int res;
    uint8_t data[1];

    data[0] = 0;


    if (isConnected)
    {
        Command *cmd=new Command(CMD_PW_SAFE_INIT_KEY,data,1);
        res=sendCommand(cmd);

        if (res==-1)
            return ERR_SENDING;
        else
        {  //sending the command was successful
            Sleep::msleep(500);
            Response *resp=new Response();
            resp->getResponse(this);

            if (cmd->crc==resp->lastCommandCRC)
            {
                if (resp->lastCommandStatus == CMD_STATUS_OK)
                {
                    return (ERR_NO_ERROR);
                }
                else
                {
                    return (ERR_STATUS_NOT_OK);
                }
            }
            else
                return ERR_WRONG_RESPONSE_CRC;
        }
    }
    return ERR_NOT_CONNECTED;
}

/*******************************************************************************

  passwordSafeSendSlotDataViaHID

  Changes
  Date      Author        Info
  01.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::passwordSafeSendSlotDataViaHID (int Slot, int Kind)
{
    int res;
    uint8_t data[2];

    data[0] = Slot;
    data[1] = Kind;

    if (isConnected)
    {
        Command *cmd=new Command(CMD_PW_SAFE_SEND_DATA,data,2);
        res=sendCommand(cmd);

        if (res==-1)
            return ERR_SENDING;
        else
        {  //sending the command was successful
            Sleep::msleep(200);
            Response *resp=new Response();
            resp->getResponse(this);

            if (cmd->crc==resp->lastCommandCRC)
            {
                if (resp->lastCommandStatus == CMD_STATUS_OK)
                {
                    return (ERR_NO_ERROR);
                }
                else
                {
                    return (ERR_STATUS_NOT_OK);
                }
            }
            else
                return ERR_WRONG_RESPONSE_CRC;
        }
    }
    return ERR_NOT_CONNECTED;
}


/*******************************************************************************

  getHighwaterMarkFromSdCard

  Get the higest accessed SD blocks in % of SD size
  (Only sice last poweron)

  Changes
  Date      Author        Info
  09.10.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

#define DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MIN         0
#define DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MAX         1
#define DEVICE_HIGH_WATERMARK_SD_CARD_READ_MIN          2
#define DEVICE_HIGH_WATERMARK_SD_CARD_READ_MAX          3

int Device::getHighwaterMarkFromSdCard (unsigned char *WriteLevelMin,unsigned char *WriteLevelMax, unsigned char *ReadLevelMin, unsigned char *ReadLevelMax)
{
    int res;
    uint8_t data[1];

    if (isConnected)
    {
        Command *cmd=new Command(CMD_SD_CARD_HIGH_WATERMARK,data,0);
        res=sendCommand(cmd);

        if (res==-1)
        {
            return (ERR_SENDING);
        }
        else
        {                                       //sending the command was successful
            Sleep::msleep(200);

            Response *resp=new Response();

            resp->getResponse(this);

            if (cmd->crc == resp->lastCommandCRC)
            {
                *WriteLevelMin = resp->data[DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MIN];
                *WriteLevelMax = resp->data[DEVICE_HIGH_WATERMARK_SD_CARD_WRITE_MAX];
                *ReadLevelMin  = resp->data[DEVICE_HIGH_WATERMARK_SD_CARD_READ_MIN];
                *ReadLevelMax  = resp->data[DEVICE_HIGH_WATERMARK_SD_CARD_READ_MAX];

                if (resp->lastCommandStatus == CMD_STATUS_OK)
                {
                    return (ERR_NO_ERROR);
                }
                else
                {
                    return (ERR_STATUS_NOT_OK);
                }
            }
            else
            {
                return ERR_WRONG_RESPONSE_CRC;
            }
        }
    }

    return (ERR_NOT_CONNECTED);
}

/*******************************************************************************

  getGeneralConfig

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Device::getGeneralConfig()
{
}

/*******************************************************************************

  writeGeneralConfig

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::writeGeneralConfig(uint8_t data[])
{
    int res;


    if (isConnected){
    Command *cmd=new Command(CMD_WRITE_CONFIG,data,5);
    authorize(cmd);
    res=sendCommand(cmd);

    if (res==-1)
        return ERR_SENDING;
    else{  //sending the command was successful
        Sleep::msleep(100);
        Response *resp=new Response();
        resp->getResponse(this);

        if (cmd->crc==resp->lastCommandCRC){
        if (resp->lastCommandStatus==CMD_STATUS_OK)
            return 0;
        }
        else
            return ERR_WRONG_RESPONSE_CRC;
    }
    }
    return ERR_NOT_CONNECTED;


}
/*******************************************************************************

  firstAuthenticate

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::firstAuthenticate(uint8_t cardPassword[], uint8_t tempPasswrod[])
{

    int res;
    uint8_t data[50];
    uint32_t crc;
    memcpy(data,cardPassword,25);
    memcpy(data+25,tempPasswrod,25);


    if (isConnected)
    {
        Command *cmd=new Command(CMD_FIRST_AUTHENTICATE,data,50);
        res=sendCommand(cmd);
        crc=cmd->crc;

        //remove the card password from memory
        delete cmd;
        memset(data,0,sizeof(data));

        if (res==-1)
            return -1;
        else
        {  //sending the command was successful
            //return cmd->crc;
            Sleep::msleep(1000);
            Response *resp=new Response();
            resp->getResponse(this);

            if (crc==resp->lastCommandCRC)
            { //the response was for the last command
                if (resp->lastCommandStatus==CMD_STATUS_OK)
                {
                    memcpy(password,tempPasswrod,25);
                    validPassword=true;
                    HID_Stick20Configuration_st.AdminPwRetryCount = 3;
                    return 0;
                }
                else if (resp->lastCommandStatus==CMD_STATUS_WRONG_PASSWORD)
                {
                    if (0 < HID_Stick20Configuration_st.AdminPwRetryCount)
                    {
                        HID_Stick20Configuration_st.AdminPwRetryCount--;
                    }
                    return -3;
                }

            }

        }

   }

    return -2;


}

/*******************************************************************************

  userAuthenticate

  Reviews
  Date      Reviewer        Info
  10.08.14  SN              First review

*******************************************************************************/

int Device::userAuthenticate(uint8_t cardPassword[], uint8_t tempPasswrod[])
{

    int res;
    uint8_t data[50];
    uint32_t crc;
    memcpy(data,cardPassword,25);
    memcpy(data+25,tempPasswrod,25);


    if (isConnected)
    {
        Command *cmd=new Command(CMD_USER_AUTHENTICATE,data,50);
        res=sendCommand(cmd);
        crc=cmd->crc;

        //remove the card password from memory
        delete cmd;
        memset(data,0,sizeof(data));

        if (res==-1)
            return -1;
        else
        {  //sending the command was successful
            //return cmd->crc;
            Sleep::msleep(1000);
            Response *resp=new Response();
            resp->getResponse(this);

            if (crc==resp->lastCommandCRC)
            { //the response was for the last command
                if (resp->lastCommandStatus==CMD_STATUS_OK)
                {
                    memcpy(userPassword,tempPasswrod,25);
                    validUserPassword=true;
                    return 0;
                }
                else if (resp->lastCommandStatus==CMD_STATUS_WRONG_PASSWORD)
                {
                    return -3;
                }

            }

        }

   }

    return -2;


}

/*******************************************************************************

  authorize

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int Device::authorize(Command *authorizedCmd)
{
   authorizedCmd->generateCRC();
   uint32_t crc=authorizedCmd->crc;
   uint8_t data[29];
   int res;

   memcpy(data,&crc,4);
   memcpy(data+4,password,25);

   if (isConnected){
   Command *cmd=new Command(CMD_AUTHORIZE,data,29);
   res=sendCommand(cmd);


   if (res==-1)
       return -1;
   else{

       Sleep::msleep(200);
       Response *resp=new Response();
       resp->getResponse(this);

       if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
           if (resp->lastCommandStatus==CMD_STATUS_OK){
               return 0;
           }

       }
       return -2;
   }
   }

   return -1;
}


/*******************************************************************************

  userAuthorize

  Reviews
  Date      Reviewer        Info
  10.08.14  SN              First review

*******************************************************************************/

int Device::userAuthorize(Command *authorizedCmd)
{
   authorizedCmd->generateCRC();
   uint32_t crc=authorizedCmd->crc;
   uint8_t data[29];
   int res;

   memcpy(data,&crc,4);
   memcpy(data+4,userPassword,25);

   if (isConnected){
   Command *cmd=new Command(CMD_USER_AUTHORIZE,data,29);
   res=sendCommand(cmd);


   if (res==-1)
       return -1;
   else{

       Sleep::msleep(200);
       Response *resp=new Response();
       resp->getResponse(this);

       if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
           if (resp->lastCommandStatus==CMD_STATUS_OK){
               return 0;
           }

       }
       return -2;
   }
   }

   return -1;
}



/*******************************************************************************

  unlockUserPassword

  Changes
  Date      Author        Info
  02.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::unlockUserPassword (uint8_t *adminPassword)
{
    int res;

    if (isConnected)
    {
        Command *cmd=new Command (CMD_UNLOCK_USER_PASSOWRD,adminPassword,30);

        res=sendCommand(cmd);

        if (res==-1)
        {
            return ERR_SENDING;
        }
        else                    //sending the command was successful
        {
            Sleep::msleep(800);
            Response *resp=new Response();
            resp->getResponse(this);

            if (cmd->crc==resp->lastCommandCRC)
            {
                if (resp->lastCommandStatus==CMD_STATUS_OK)
                {
                    HID_Stick20Configuration_st.UserPwRetryCount = 3;
                    Stick20_ConfigurationChanged = TRUE;
                    return 0;
                }
            }
            else
            {
                return ERR_WRONG_RESPONSE_CRC;
            }
        }
    }
    return ERR_NOT_CONNECTED;
}


/*******************************************************************************

  isAesSupported

  Changes
  Date      Author        Info
  27.10.14  GG            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::isAesSupported(uint8_t* password)
{
    int res;

    if (isConnected)
    {
        Command *cmd=new Command (CMD_DETECT_SC_AES, password, strlen((const char*)password));

        res = sendCommand(cmd);

        if (-1 == res)
        {
            return ERR_SENDING;
        }
        else                    //sending the command was successful
        {
            Sleep::msleep(2000);
            Response *resp=new Response();
            resp->getResponse(this);

            if (cmd->crc == resp->lastCommandCRC)
            {
                if (CMD_STATUS_OK == resp->lastCommandStatus) {
 //                   validUserPassword = true;
                }
                return resp->lastCommandStatus;
            }
            else
            {
                return ERR_WRONG_RESPONSE_CRC;
            }
        }
    }
    return ERR_NOT_CONNECTED;
}



/*******************************************************************************

    Here starts the new commands for stick 2.0

*******************************************************************************/

/*******************************************************************************

  stick20EnableCryptedPartition

  Send the command

    STICK20_CMD_ENABLE_CRYPTED_PARI

  with the password to cryptostick 2.0

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

#define CS20_MAX_PASSWORD_LEN       30

bool Device::stick20EnableCryptedPartition  (uint8_t *password)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check password length
    n = strlen ((const char*)password);
    if (CS20_MAX_PASSWORD_LEN <= n)
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_ENABLE_CRYPTED_PARI,password,n);
    res = sendCommand(cmd);

    if(res){}//Fix warnings
    return (true);
}
/*******************************************************************************

  stick20DisableCryptedPartition

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20DisableCryptedPartition  (void)
{
    int      res;
    Command *cmd;

    cmd = new Command(STICK20_CMD_DISABLE_CRYPTED_PARI,NULL,0);
    res = sendCommand(cmd);

    if(res){}//Fix warnings
    return (true);
}

/*******************************************************************************

  stick20EnableHiddenCryptedPartition

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20EnableHiddenCryptedPartition  (uint8_t *password)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check password length
    n = strlen ((const char*)password);

    if (CS20_MAX_PASSWORD_LEN <= n)
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI,password,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20DisableHiddenCryptedPartition

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20DisableHiddenCryptedPartition  (void)
{
    int      res;
    Command *cmd;

    cmd = new Command(STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI,NULL,0);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20EnableFirmwareUpdate

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20EnableFirmwareUpdate (uint8_t *password)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check password length
    n = strlen ((const char*)password);
    if (CS20_MAX_PASSWORD_LEN <= n)
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_ENABLE_FIRMWARE_UPDATE,password,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20ExportFirmware

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20ExportFirmware (uint8_t *password)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check password length
    n = strlen ((const char*)password);
    if (CS20_MAX_PASSWORD_LEN <= n)
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_EXPORT_FIRMWARE_TO_FILE,password,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20CreateNewKeys

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20CreateNewKeys (uint8_t *password)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check password length
    n = strlen ((const char*)password);
    if (CS20_MAX_PASSWORD_LEN <= n)
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_GENERATE_NEW_KEYS,password,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20FillSDCardWithRandomChars

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20FillSDCardWithRandomChars (uint8_t *password,uint8_t VolumeFlag)
{
    uint8_t  n;
    int      res;
    Command *cmd;
    uint8_t  data[CS20_MAX_PASSWORD_LEN+2];

    // Check password length
    n = strlen ((const char*)password);
    if (CS20_MAX_PASSWORD_LEN <= n)
    {
        return (false);
    }

    data[0] = VolumeFlag;
    strcpy ((char*)&data[1],(char*)password);

    cmd = new Command(STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS,data,n+1);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SetupHiddenVolume

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20SetupHiddenVolume (void)
{
//    uint8_t n;
    int     res;
    Command *cmd;
//    Response *resp;

    cmd = new Command(STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP,NULL,0);
    res = sendCommand(cmd);


/*
    Sleep::msleep(200);
    Response *resp=new Response();
    resp->getResponse(this);

    if (cmd->crc==resp->lastCommandCRC)
    { //the response was for the last command
        if (resp->lastCommandStatus!=CMD_STATUS_OK){
            return (FALSE);
        }
    }
*/
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20GetPasswordMatrix

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20GetPasswordMatrix (void)
{
//    uint8_t n;
    int     res;
    Command *cmd;
//    Response *resp;

    cmd = new Command(STICK20_CMD_SEND_PASSWORD_MATRIX,NULL,0);
    res = sendCommand(cmd);

/*

    Sleep::msleep(200);
    Response *resp=new Response();
    resp->getResponse(this);

    if (cmd->crc==resp->lastCommandCRC)
    { //the response was for the last command
        if (resp->lastCommandStatus==CMD_STATUS_OK){
            return 0;
        }
    }
*/
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SendPasswordMatrixPinData

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20SendPasswordMatrixPinData (uint8_t *Pindata)
{
    uint8_t n;
    int     res;
    Command *cmd;
//    Response *resp;

    // Check pin data length
    n = strlen ((const char*)Pindata);
    if (STICK20_PASSOWRD_LEN + 2 <= n)      // Kind byte + End byte 0
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA,Pindata,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SendPasswordMatrixSetup

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20SendPasswordMatrixSetup (uint8_t *Setupdata)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check pin data length
    n = strlen ((const char*)Setupdata);
    if (STICK20_PASSOWRD_LEN + 1 <= n)
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP,Setupdata,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20GetStatusData

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool Device::stick20GetStatusData ()
{
    int     res;
    Command *cmd;

    HID_Stick20Init ();         // Clear data

    cmd = new Command(STICK20_CMD_GET_DEVICE_STATUS,NULL,0);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SendPassword

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int Device::stick20SendPassword (uint8_t *Pindata)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check pin data length
    n = strlen ((const char*)Pindata);
    if (STICK20_PASSOWRD_LEN + 2 <= n)      // Kind byte + End byte 0
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_SEND_PASSWORD,Pindata,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SendNewPassword

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int Device::stick20SendNewPassword (uint8_t *NewPindata)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check pin data length
    n = strlen ((const char*)NewPindata);
    if (STICK20_PASSOWRD_LEN + 2 <= n)      // Kind byte + End byte 0
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_SEND_NEW_PASSWORD,NewPindata,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SendSetReadonlyToUncryptedVolume

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int Device::stick20SendSetReadonlyToUncryptedVolume (uint8_t *Pindata)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check pin data length
    n = strlen ((const char*)Pindata);
    if (STICK20_PASSOWRD_LEN + 2 <= n)      // Kind byte + End byte 0
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN,Pindata,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SendSetReadwriteToUncryptedVolume

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int Device::stick20SendSetReadwriteToUncryptedVolume (uint8_t *Pindata)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check pin data length
    n = strlen ((const char*)Pindata);
    if (STICK20_PASSOWRD_LEN + 2 <= n)      // Kind byte + End byte 0
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN,Pindata,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SendClearNewSdCardFound

  Changes
  Date      Author          Info
  11.02.14  RB              Implementation: New SD card found

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20SendClearNewSdCardFound (uint8_t *Pindata)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check pin data length
    n = strlen ((const char*)Pindata);
    if (STICK20_PASSOWRD_LEN + 2 <= n)      // Kind byte + End byte 0
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND,Pindata,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}

/*******************************************************************************

  stick20SendStartup

  Changes
  Date      Author          Info
  28.03.14  RB              Send startup and get startup infos

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20SendStartup (uint64_t localTime)
{
    uint8_t   data[30];
    int       res;
    Command  *cmd;

    memcpy (data,&localTime,8);

    cmd = new Command (STICK20_CMD_SEND_STARTUP,data,8);
    res = sendCommand (cmd);
    if(res){}//Fix warnings

    return (TRUE);
}

/*******************************************************************************

  stick20SendHiddenVolumeSetup

  Changes
  Date      Author          Info
  25.04.14  RB              Send setup of a hidden volume

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20SendHiddenVolumeSetup (HiddenVolumeSetup_tst *HV_Data_st)
{
    uint8_t   data[30];
    int       res;
    Command  *cmd;
    //uint8_t   SizeCheck_data[1 - 2*(sizeof(HiddenVolumeSetup_tst) > 30)];

    memcpy (data,HV_Data_st,sizeof (HiddenVolumeSetup_tst));

    cmd = new Command (STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP,data,sizeof (HiddenVolumeSetup_tst));
    res = sendCommand (cmd);
    if(res){}//Fix warnings

    return (TRUE);
}

/*******************************************************************************

  stick20LockFirmware

  Changes
  Date      Author        Info
  02.06.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20LockFirmware (uint8_t *password)
{
    uint8_t n;
    int     res;
    Command *cmd;

    // Check password length
    n = strlen ((const char*)password);
    if (CS20_MAX_PASSWORD_LEN <= n)
    {
        return (false);
    }

    cmd = new Command(STICK20_CMD_SEND_LOCK_STICK_HARDWARE,password,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}



/*******************************************************************************

  stick20ProductionTest

  Changes
  Date      Author        Info
  07.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int Device::stick20ProductionTest (void)
{
    uint8_t n;
    int     res;
    uint8_t TestData[10];
    Command *cmd;

    n = 0;

    cmd = new Command(STICK20_CMD_PRODUCTION_TEST,TestData,n);
    res = sendCommand(cmd);
    if(res){}//Fix warnings

    return (true);
}


/*******************************************************************************

  getCode

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/
/*
int Device::getCode(uint8_t slotNo, uint64_t challenge,uint64_t lastTOTPTime,uint8_t  lastInterval,uint8_t result[18])
{

    qDebug() << "getting code" << slotNo;
    int res;
    uint8_t data[30];

    data[0]=slotNo;

    memcpy(data+ 1,&challenge,8);

    memcpy(data+ 9,&lastTOTPTime,8);     // Time of challenge: Warning: it's better to tranfer time and interval, to avoid attacks with wrong timestamps
    memcpy(data+17,&lastInterval,1);

    if (isConnected){
       qDebug() << "sending command";

    Command *cmd=new Command(CMD_GET_CODE,data,18);
    res=sendCommand(cmd);

    if (res==-1)
        return -1;
    else{  //sending the command was successful
        //return cmd->crc;
         qDebug() << "command sent";
        Sleep::msleep(100);
        Response *resp=new Response();
        resp->getResponse(this);


        if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
            if (resp->lastCommandStatus==CMD_STATUS_OK){
                memcpy(result,resp->data,18);

            }

        }

        return 0;
    }

   }

    return -1;

}
*/

/*******************************************************************************

  testHOTP

  Reviews
  Date      Reviewer        Info
  06.08.14  SN              First review

*******************************************************************************/
//START - OTP Test Routine --------------------------------
/*
uint16_t Device::testHOTP(uint16_t tests_number,uint8_t counter_number){

    uint8_t data[10];
    uint16_t result;
    int res;

    data[0] =counter_number;

    memcpy(data+1,&tests_number,2);

    if (isConnected){
       qDebug() << "sending command";

    Command *cmd=new Command(CMD_TEST_COUNTER,data,3);
    res=sendCommand(cmd);

    if (res==-1)
        return -2;
    else{  //sending the command was successful
        //return cmd->crc;
         qDebug() << "command sent";
        Sleep::msleep(100*tests_number);
        Response *resp=new Response();
        resp->getResponse(this);


        if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
            if (resp->lastCommandStatus==CMD_STATUS_OK){
                //memcpy(&result,resp->data,2);
                result = *((uint16_t *)resp->data);
                return result;
            }

        }

        return -1;
    }

   }

    return -2;

}
*/
//END - OTP Test Routine ----------------------------------

/*******************************************************************************

  testTOTP

  Reviews
  Date      Reviewer        Info
  06.08.14  SN              First review

*******************************************************************************/

//START - OTP Test Routine --------------------------------
/*
uint16_t Device::testTOTP(uint16_t tests_number){

    uint8_t data[10];
    uint16_t result;
    int res;

    data[0] = 1;

    memcpy(data+1,&tests_number,2);

    if (isConnected){
       qDebug() << "sending command";

    Command *cmd=new Command(CMD_TEST_TIME,data,3);
    res=sendCommand(cmd);

    if (res==-1)
        return -2;
    else{  //sending the command was successful
        //return cmd->crc;
         qDebug() << "command sent";
        Sleep::msleep(100*tests_number);
        Response *resp=new Response();
        resp->getResponse(this);


        if (cmd->crc==resp->lastCommandCRC){ //the response was for the last command
            if (resp->lastCommandStatus==CMD_STATUS_OK){
                //memcpy(&result,resp->data,2);
                result = *((uint16_t *)resp->data);
                return result;
            }

        }

        return -1;
    }

   }

    return -2;

}
*/
//END - OTP Test Routine ----------------------------------
