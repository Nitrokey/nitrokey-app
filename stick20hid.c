/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-12
*
* This file is part of GPF Crypto Stick 2
*
* GPF Crypto Stick 2  is free software: you can redistribute it and/or modify
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
#include <stdlib.h>
#include <string.h>

#include "stick20hid.h"

#define FALSE               0
#define TRUE                1


/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 Global declarations

*******************************************************************************/

char  DebugText_Stick20[STICK20_DEBUG_TEXT_LEN];
char  DebugNewText[STICK20_DEBUG_TEXT_LEN];               // We have it

int   DebugTextlen_Stick20          = 0;
int   DebugNewTextLen               = 0;
char  DebugTextHasChanged           = FALSE;
int   DebugingActive                = FALSE;
int   DebugingStick20PoolingActive  = FALSE;
int   DebugingFileStickActive       = FALSE;
int   DebugingFileGuiActive         = FALSE;
char  DebugingStickFilename[256];
char  DebugingGuiFilename[256];

/*
extern int  DebugingFileStickActive;
extern int  DebugingFileGuiActive;
extern char DebugingStickFilename[256];
extern char DebugingGuiFilename[256];
*/
HID_Stick20SendData_est             HID_Stick20ReceiveData_st;

HID_Stick20MatrixPasswordData_est   HID_Stick20MatrixPasswordData_st;

int Stick20_ConfigurationChanged = FALSE;
volatile typeStick20Configuration_st         HID_Stick20Configuration_st;

int Stick20_ProductionInfosChanged = FALSE;
typeStick20ProductionInfos_st Stick20ProductionInfos_st;

/*******************************************************************************

 External declarations

*******************************************************************************/

/** Only for debugging */


/*******************************************************************************

  DebugInitDebugging

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void DebugInitDebugging (void)
{
    DebugText_Stick20[0] = 0;
    DebugTextlen_Stick20 = 0;
    DebugNewText[0]      = 0;
    DebugNewTextLen      = 0;

    DebugTextHasChanged  = FALSE;

    strcpy (DebugingStickFilename,"Firmwarelog.txt");
    strcpy (DebugingGuiFilename,"Guilog.txt");
}

/*******************************************************************************

  DebugAppendText

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void DebugAppendText (char *Text)
{
    int i;

    if (FALSE == DebugingActive)            // Don't save text when debugging is disabled
    {
        return;
    }

    if (STICK20_DEBUG_TEXT_LEN <= DebugTextlen_Stick20 + strlen (Text) + DebugNewTextLen - 1)
    {
        return;
    }

    i = 0;
    while (Text[i] != 0)
    {
        // Remove embedded LF
        if ('\r' != Text[i])
        {
            DebugNewText[DebugNewTextLen] = Text[i];
            DebugNewTextLen++;
        }
        i++;
    }
    DebugNewText[DebugNewTextLen] = 0;

    if (0 != i)
    {
        DebugTextHasChanged = TRUE;
    }
}

/*******************************************************************************

  DebugAppendFileStickText
  
  Write text in to logfile

  Changes
  Date      Author        Info
  22.10.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void DebugAppendFileStickText (char *Text)
{
    FILE *fp;
    int i;

    if (FALSE == DebugingFileStickActive)            // Don't save text when debugging is disabled
    {
        return;
    }

    fp = fopen (DebugingStickFilename,"a+");
    if (NULL == fp)
    {
        return;
    }

    i = 0;
    while (Text[i] != 0)
    {
        if ('\r' != Text[i])        // Remove embedded LF
        {
            fputc (Text[i],fp);
        }
        i++;
    }
    fclose (fp);
}

/*******************************************************************************

  DebugAppendFileGuiText
  
  Write text in to logfile

  Changes
  Date      Author        Info
  22.10.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void DebugAppendFileGuiText (char *Text)
{
    FILE *fp;
    int i;

    if (FALSE == DebugingFileGuiActive)            // Don't save text when debugging is disabled
    {
        return;
    }

    fp = fopen (DebugingGuiFilename,"a+");
    if (NULL == fp)
    {
        return;
    }


    i = 0;
    while (Text[i] != 0)
    {
        if ('\r' != Text[i])        // Remove embedded LF
        {
            fputc (Text[i],fp);
        }
        i++;
    }
    fclose (fp);
}


/** Only for debugging - End */
/*******************************************************************************

  HID_Stick20Init

  Changes
  Date      Author        Info
  09.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void HID_Stick20Init (void)
{
    HID_Stick20Configuration_st.MagicNumber_StickConfig_u16     = 0;
    HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8 = 0;
    HID_Stick20Configuration_st.ReadWriteFlagCryptedVolume_u8   = 0;
    memcpy ((void*)HID_Stick20Configuration_st.VersionInfo_au8,"----",4);
    HID_Stick20Configuration_st.ReadWriteFlagHiddenVolume_u8    = 0;
    HID_Stick20Configuration_st.FirmwareLocked_u8               = 0;
    HID_Stick20Configuration_st.NewSDCardFound_u8               = 0;
    HID_Stick20Configuration_st.SDFillWithRandomChars_u8        = 0;
    HID_Stick20Configuration_st.ActiveSD_CardID_u32             = 0;
    HID_Stick20Configuration_st.VolumeActiceFlag_u8             = 0;
    HID_Stick20Configuration_st.NewSmartCardFound_u8            = 0;
    HID_Stick20Configuration_st.UserPwRetryCount                = 99;
    HID_Stick20Configuration_st.AdminPwRetryCount               = 99;
    HID_Stick20Configuration_st.ActiveSmartCardID_u32           = 0;
    HID_Stick20Configuration_st.StickKeysNotInitiated           = 0;
}

/*******************************************************************************

  HID_GetStick20Configuration

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int HID_GetStick20Configuration (void)
{
    char text[50];
//    unsigned char NewDebugBlock = 1;
//    int len;
    static typeStick20Configuration_st SavedConfiguration_st;

    DebugAppendText ("GetStick20Configuration\n");

//    NewDebugBlock = HID_Stick20ReceiveData_st.SendCounter_u8;
//    len = HID_Stick20ReceiveData_st.SendSize_u8;

    memcpy ((void*)&HID_Stick20Configuration_st,
                   &HID_Stick20ReceiveData_st.SendData_u8[0],
                   sizeof (HID_Stick20Configuration_st));

    if (0 != memcmp((void*)&HID_Stick20Configuration_st,(void*)&SavedConfiguration_st,sizeof (typeStick20Configuration_st)))
    {
        Stick20_ConfigurationChanged = TRUE;
        SavedConfiguration_st = HID_Stick20Configuration_st;
    }

    snprintf(text,sizeof (text),"HID_GetStick20Configuration\n" );                 DebugAppendText (text);

    snprintf(text,sizeof (text),"MagicNumber_StickConfig_u16      : %d\n",HID_Stick20Configuration_st.MagicNumber_StickConfig_u16      );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"ReadWriteFlagUncryptedVolume_u8  : %d\n",HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8  );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"ReadWriteFlagCryptedVolume_u8    : %d\n",HID_Stick20Configuration_st.ReadWriteFlagCryptedVolume_u8    );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"VersionInfo_au8[4]               : %d %d %d %d\n",HID_Stick20Configuration_st.VersionInfo_au8[0],HID_Stick20Configuration_st.VersionInfo_au8[1],HID_Stick20Configuration_st.VersionInfo_au8[2],HID_Stick20Configuration_st.VersionInfo_au8[3]                  );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"ReadWriteFlagHiddenVolume_u8     : %d\n",HID_Stick20Configuration_st.ReadWriteFlagHiddenVolume_u8     );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"FirmwareLocked_u8                : %d\n",HID_Stick20Configuration_st.FirmwareLocked_u8                );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"NewSDCardFound_u8                : %d\n",HID_Stick20Configuration_st.NewSDCardFound_u8                );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"SDFillWithRandomChars_u8         : %d\n",HID_Stick20Configuration_st.SDFillWithRandomChars_u8         );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"ActiveSD_CardID_u32              : 0x%08X\n",HID_Stick20Configuration_st.ActiveSD_CardID_u32              );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"VolumeActiceFlag_u8              : %d\n",HID_Stick20Configuration_st.VolumeActiceFlag_u8              );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"NewSmartCardFound_u8             : %d\n",HID_Stick20Configuration_st.NewSmartCardFound_u8             );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"UserPwRetryCount                 : %d\n",HID_Stick20Configuration_st.UserPwRetryCount                 );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"AdminPwRetryCount                : %d\n",HID_Stick20Configuration_st.AdminPwRetryCount                );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"ActiveSmartCardID_u32            : 0x%X\n",HID_Stick20Configuration_st.ActiveSmartCardID_u32            );                 DebugAppendText (text);
    snprintf(text,sizeof (text),"StickKeysNotInitiated            : %d\n",HID_Stick20Configuration_st.StickKeysNotInitiated            );                 DebugAppendText (text);


//    if(len){}//Fix warnings
//    if(NewDebugBlock){}//Fix warnings

    return (TRUE);
}

/*******************************************************************************

  HID_GetStick20ProductionInfos

  Changes
  Date      Author        Info
  07.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int HID_GetStick20ProductionInfos (void)
{
    unsigned char NewDebugBlock = 1;
    int len;
    static typeStick20ProductionInfos_st SavedProductionInfos_st;

    DebugAppendText ("GetStick20ProductionInfos");

    NewDebugBlock = HID_Stick20ReceiveData_st.SendCounter_u8;
    len = HID_Stick20ReceiveData_st.SendSize_u8;

    memcpy (&Stick20ProductionInfos_st,
            &HID_Stick20ReceiveData_st.SendData_u8[0],
            sizeof (typeStick20ProductionInfos_st));

    if (0 != memcmp((void*)&Stick20ProductionInfos_st,(void*)&SavedProductionInfos_st,sizeof (typeStick20ProductionInfos_st)))
    {
        Stick20_ProductionInfosChanged = TRUE;
        SavedProductionInfos_st = Stick20ProductionInfos_st;
    }
    if(len){}//Fix warnings
    if(NewDebugBlock){}//Fix warnings

    return (TRUE);
}

/*******************************************************************************

  HID_GetStick20PasswordMatrixData

  Password matrix HID Block

  Byte
  0       Status
          0 - IDEL
          1 - Start new Block
          2 - Data  0-19
          3 - Data 20-39
          4 - Data 40-59
          5 - Data 60-79
          6 - Data 80-99
          7 - All Data send, CRC
  1-20    Data or CRC

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int HID_GetStick20PasswordMatrixData (void)
{
    static unsigned char LastDebugBlock = 0;
    unsigned char NewDebugBlock = 1;
    int len;

    NewDebugBlock = HID_Stick20ReceiveData_st.SendCounter_u8;
    len = HID_Stick20ReceiveData_st.SendSize_u8;

    if ((LastDebugBlock != NewDebugBlock) && (len != 0))
    {
        switch (HID_Stick20ReceiveData_st.SendData_u8[0])
        {
            case 0 :
                break;
            case 1 :
                DebugAppendText ("GetStick20PasswordMatrixData 1\n");
                HID_Stick20MatrixPasswordData_st.StatusFlag_u8 = STICK20_PASSWORD_MATRIX_STATUS_GET_NEW_BLOCK;
                memset (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[0],-1,STICK20_PASSWORD_MATRIX_DATA_LEN);
                break;
            case 2 :
                DebugAppendText ("GetStick20PasswordMatrixData 2\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[0],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 3 :
                DebugAppendText ("GetStick20PasswordMatrixData 3\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[20],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 4 :
                DebugAppendText ("GetStick20PasswordMatrixData 4\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[40],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 5 :
                DebugAppendText ("GetStick20PasswordMatrixData 5\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[60],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 6 :
                DebugAppendText ("GetStick20PasswordMatrixData 6\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[80],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 7 :
                DebugAppendText ("GetStick20PasswordMatrixData 7 - All in\n");
                HID_Stick20MatrixPasswordData_st.StatusFlag_u8 = STICK20_PASSWORD_MATRIX_STATUS_NEW_BLOCK_RECEIVED;
                break;
            default :
                break;
        }

        LastDebugBlock = HID_Stick20ReceiveData_st.SendCounter_u8;
    }
    return (TRUE);
}

/*******************************************************************************

  HID_GetStick20DebugData

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int HID_GetStick20DebugData (void)
{
    static unsigned char LastDebugBlock = 0;
    unsigned char NewDebugBlock = 1;
    int len;

    NewDebugBlock = HID_Stick20ReceiveData_st.SendCounter_u8;
    len = HID_Stick20ReceiveData_st.SendSize_u8;

    if ((LastDebugBlock != NewDebugBlock) && (len != 0))
    {
        if (OUTPUT_CMD_STICK20_SEND_DATA_SIZE < len)
        {
            len = OUTPUT_CMD_STICK20_SEND_DATA_SIZE;
        }
        HID_Stick20ReceiveData_st.SendData_u8[len] = 0;
        
        
        DebugAppendText ((char *)&HID_Stick20ReceiveData_st.SendData_u8[0]);

        LastDebugBlock = HID_Stick20ReceiveData_st.SendCounter_u8;
    }
    return (TRUE);
}

/*******************************************************************************

  HID_GetStick20ReceiveData

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

int HID_GetStick20ReceiveData (unsigned char *data)
{
    //static unsigned char LastDebugBlock = 0;
    //unsigned char NewDebugBlock = 1;

    /* Copy Stick 2.0 HID response to receive struct */
#ifdef WIN32
    memcpy ((void*)&HID_Stick20ReceiveData_st,data+1+OUTPUT_CMD_RESULT_STICK20_DATA_START,sizeof (HID_Stick20ReceiveData_st));
#endif

#ifdef linux
    memcpy ((void*)&HID_Stick20ReceiveData_st,data+OUTPUT_CMD_RESULT_STICK20_DATA_START,sizeof (HID_Stick20ReceiveData_st));
#endif

#ifdef MAC
    To Check ....
    memcpy ((void*)&HID_Stick20ReceiveData_st,data+OUTPUT_CMD_RESULT_STICK20_DATA_START,sizeof (HID_Stick20ReceiveData_st));
#endif

if (OUTPUT_CMD_STICK20_SEND_DATA_TYPE_NONE != HID_Stick20ReceiveData_st.SendDataType_u8)
{
        char text[1000];
        int i;

        snprintf(text,sizeof (text),"HID_GetStick20ReceiveData: ");
        DebugAppendText (text);
        for (i=0;i<64;i++)
        {
            snprintf(text,sizeof (text),"%02x ",data[i]);
            DebugAppendText (text);
        }
        snprintf(text,sizeof (text),"\n");
        DebugAppendText (text);

/*
        sprintf(text,"HID_GetStick20ReceiveData: SendCounter %d Typ %d - %d - Size %d\n",
                    HID_Stick20ReceiveData_st.SendCounter_u8,
                    HID_Stick20ReceiveData_st.SendDataType_u8,
                    HID_Stick20ReceiveData_st.FollowBytesFlag_u8,
                    HID_Stick20ReceiveData_st.SendSize_u8
                );

        DebugAppendText (text);
*/
}


    switch (HID_Stick20ReceiveData_st.SendDataType_u8)
    {
        case OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG :
            HID_GetStick20DebugData ();
            break;
        case OUTPUT_CMD_STICK20_SEND_DATA_TYPE_PW_DATA :
            HID_GetStick20PasswordMatrixData ();
            break;
        case OUTPUT_CMD_STICK20_SEND_DATA_TYPE_STATUS :
            HID_GetStick20Configuration ();
            break;
        case OUTPUT_CMD_STICK20_SEND_DATA_TYPE_PROD_INFO :
            HID_GetStick20ProductionInfos ();
            break;
    }

    return (TRUE);
}

