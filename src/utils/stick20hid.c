/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-12
*
* This file is part of Nitrokey 2
*
* Nitrokey 2  is free software: you can redistribute it and/or modify
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mcvs-wrapper.h"
#include "stick20hid.h"

#define FALSE               0
#define TRUE                1


/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 Global declarations

*******************************************************************************/
int   DebugingActive                = FALSE;
int   DebugingStick20PoolingActive  = FALSE;

char  DebugText_GUI[STICK20_DEBUG_TEXT_LEN];
char  DebugNewText_GUI[STICK20_DEBUG_TEXT_LEN];

char  DebugingGuiFilename[256];
int   DebugingFileGuiActive         = FALSE;
int   DebugTextlen_GUI              = 0;
int   DebugNewTextLen_GUI           = 0;
char  DebugTextHasChanged_GUI       = FALSE;

/* Debug output fom stick 20 via HID interface */

char  DebugText_Stick20[STICK20_DEBUG_TEXT_LEN];
char  DebugNewText_Stick20[STICK20_DEBUG_TEXT_LEN];

char  DebugingStickFilename[256];
int   DebugingFileStickActive       = FALSE;
int   DebugTextlen_Stick20          = 0;
int   DebugNewTextLen_Stick20       = 0;
char  DebugTextHasChanged_Stick20   = FALSE;



/*
extern int  DebugingFileStickActive;
extern int  DebugingFileGuiActive;
extern char DebugingStickFilename[256];
extern char DebugingGuiFilename[256];
*/
HID_Stick20SendData_est             HID_Stick20ReceiveData_st;

HID_Stick20MatrixPasswordData_est   HID_Stick20MatrixPasswordData_st;

int Stick20_ConfigurationChanged = FALSE;
volatile typeStick20Configuration_st HID_Stick20Configuration_st;
volatile typeStick20Configuration_st SavedConfiguration_st;

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
    DebugText_GUI[0] = 0;
    DebugTextlen_GUI = 0;
    DebugNewText_GUI[0]      = 0;
    DebugNewTextLen_GUI      = 0;

    DebugTextHasChanged_GUI  = FALSE;

    STRCPY (DebugingStickFilename,sizeof (DebugingStickFilename),"Firmwarelog.txt");
    STRCPY (DebugingGuiFilename,sizeof (DebugingGuiFilename),"Guilog.txt");
}


/*******************************************************************************

  DebugAppendTextStick

  Changes
  Date      Author        Info
  08.12.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

char *GetTimeStampForLog (void);

void DebugAppendTimestampToLog (void)
{
    char *OutputString;
    char CrString[2] = "\n";

    OutputString = GetTimeStampForLog ();

    if (0 == strlen(OutputString))
    {
        return;
    }

    DebugAppendTextGui_NoTimeStamp ("*** ");
    DebugAppendTextGui_NoTimeStamp (OutputString);
    DebugAppendTextGui_NoTimeStamp (" ***\n");

    DebugAppendTextStick_NoTimeStamp ("*** ");
    DebugAppendTextStick_NoTimeStamp (OutputString);
    DebugAppendTextStick_NoTimeStamp (" ***\n");
}

/*******************************************************************************

  DebugAppendTextGui

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void DebugAppendTextGui (const char *Text)
{
    int i;
    static int LastCharWasCr = FALSE;

    if (FALSE == DebugingActive)            // Don't save text when debugging is disabled
    {
        return;
    }

    if (STICK20_DEBUG_TEXT_LEN <= DebugTextlen_GUI + strlen (Text) + DebugNewTextLen_GUI - 1)
    {
        return;
    }

    i = 0;
    while (Text[i] != 0)
    {
        if (TRUE == LastCharWasCr)
        {
            LastCharWasCr = FALSE;
            DebugAppendTimestampToLog ();
        }

        // Remove embedded LF
        if ('\r' != Text[i])
        {
            DebugNewText_GUI[DebugNewTextLen_GUI] = Text[i];
            DebugNewTextLen_GUI++;
        }

        // Check for writing timestamp, only after CR
        if ('\n' == Text[i])
        {
            LastCharWasCr = TRUE;
        }
        i++;
    }
    DebugNewText_GUI[DebugNewTextLen_GUI] = 0;

    if (0 != i)
    {
        DebugTextHasChanged_GUI = TRUE;
    }
}

/*******************************************************************************

  DebugAppendTextGui

  Changes
  Date      Author        Info
  08.12.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void DebugAppendTextGui_NoTimeStamp (char *Text)
{
    int i;

    if (FALSE == DebugingActive)            // Don't save text when debugging is disabled
    {
        return;
    }

    if (STICK20_DEBUG_TEXT_LEN <= DebugTextlen_GUI + strlen (Text) + DebugNewTextLen_GUI - 1)
    {
        return;
    }

    i = 0;
    while (Text[i] != 0)
    {
        // Remove embedded LF
        if ('\r' != Text[i])
        {
            DebugNewText_GUI[DebugNewTextLen_GUI] = Text[i];
            DebugNewTextLen_GUI++;
        }
        i++;
    }
    DebugNewText_GUI[DebugNewTextLen_GUI] = 0;

    if (0 != i)
    {
        DebugTextHasChanged_GUI = TRUE;
    }
}


/*******************************************************************************

  DebugAppendTextStick

  Changes
  Date      Author        Info
  08.12.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void DebugAppendTextStick (char *Text)
{
    int i;
    static int LastCharWasCr = FALSE;

    if (FALSE == DebugingActive)            // Don't save text when debugging is disabled
    {
        return;
    }

    if (STICK20_DEBUG_TEXT_LEN <= DebugTextlen_Stick20 + strlen (Text) + DebugNewTextLen_Stick20 - 1)
    {
        return;
    }

    i = 0;
    while (Text[i] != 0)
    {
        if (TRUE == LastCharWasCr)
        {
            LastCharWasCr = FALSE;
            DebugAppendTimestampToLog ();
        }

        // Remove embedded LF
        if ('\r' != Text[i])
        {
            DebugNewText_Stick20[DebugNewTextLen_Stick20] = Text[i];
            DebugNewTextLen_Stick20++;
        }

        // Check for writing timestamp, only after CR
        if ('\n' == Text[i])
        {
            LastCharWasCr = TRUE;
        }
        i++;
    }
    DebugNewText_Stick20[DebugNewTextLen_Stick20] = 0;

    if (0 != i)
    {
        DebugTextHasChanged_Stick20 = TRUE;
    }
}


/*******************************************************************************

  DebugAppendTextStick_NoTimeStamp

  Changes
  Date      Author        Info
  08.12.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void DebugAppendTextStick_NoTimeStamp (char *Text)
{
    int i;

    if (FALSE == DebugingActive)            // Don't save text when debugging is disabled
    {
        return;
    }

    if (STICK20_DEBUG_TEXT_LEN <= DebugTextlen_Stick20 + strlen (Text) + DebugNewTextLen_Stick20 - 1)
    {
        return;
    }

    i = 0;
    while (Text[i] != 0)
    {
        // Remove embedded LF
        if ('\r' != Text[i])
        {
            DebugNewText_Stick20[DebugNewTextLen_Stick20] = Text[i];
            DebugNewTextLen_Stick20++;
        }
        i++;
    }
    DebugNewText_Stick20[DebugNewTextLen_Stick20] = 0;

    if (0 != i)
    {
        DebugTextHasChanged_Stick20 = TRUE;
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


    FOPEN(fp, DebugingStickFilename,"a+");
    if (0 == fp)
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

    FOPEN(fp,DebugingGuiFilename,"a+");
    if (0 ==  fp)
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
    Stick20_ConfigurationChanged = FALSE;

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

    SavedConfiguration_st.MagicNumber_StickConfig_u16           = 1;        // Flag for a difference between new and old config

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

    DebugAppendTextGui ("GetStick20Configuration\n");

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


    SNPRINTF(text,sizeof (text),"HID_GetStick20Configuration\n" );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"MagicNumber_StickConfig_u16      : %d\n",HID_Stick20Configuration_st.MagicNumber_StickConfig_u16      );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"ReadWriteFlagUncryptedVolume_u8  : %d\n",HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8  );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"ReadWriteFlagCryptedVolume_u8    : %d\n",HID_Stick20Configuration_st.ReadWriteFlagCryptedVolume_u8    );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"VersionInfo_au8[4]               : %d %d %d %d\n",HID_Stick20Configuration_st.VersionInfo_au8[0],HID_Stick20Configuration_st.VersionInfo_au8[1],HID_Stick20Configuration_st.VersionInfo_au8[2],HID_Stick20Configuration_st.VersionInfo_au8[3]                  );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"ReadWriteFlagHiddenVolume_u8     : %d\n",HID_Stick20Configuration_st.ReadWriteFlagHiddenVolume_u8     );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"FirmwareLocked_u8                : %d\n",HID_Stick20Configuration_st.FirmwareLocked_u8                );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"NewSDCardFound_u8                : %d\n",HID_Stick20Configuration_st.NewSDCardFound_u8                );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"SDFillWithRandomChars_u8         : %d\n",HID_Stick20Configuration_st.SDFillWithRandomChars_u8         );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"ActiveSD_CardID_u32              : 0x%08X\n",HID_Stick20Configuration_st.ActiveSD_CardID_u32              );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"VolumeActiceFlag_u8              : %d\n",HID_Stick20Configuration_st.VolumeActiceFlag_u8              );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"NewSmartCardFound_u8             : %d\n",HID_Stick20Configuration_st.NewSmartCardFound_u8             );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"UserPwRetryCount                 : %d\n",HID_Stick20Configuration_st.UserPwRetryCount                 );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"AdminPwRetryCount                : %d\n",HID_Stick20Configuration_st.AdminPwRetryCount                );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"ActiveSmartCardID_u32            : 0x%X\n",HID_Stick20Configuration_st.ActiveSmartCardID_u32            );                 DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"StickKeysNotInitiated            : %d\n",HID_Stick20Configuration_st.StickKeysNotInitiated            );                 DebugAppendTextGui (text);


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

    DebugAppendTextGui ("GetStick20ProductionInfos");

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
                DebugAppendTextGui ("GetStick20PasswordMatrixData 1\n");
                HID_Stick20MatrixPasswordData_st.StatusFlag_u8 = STICK20_PASSWORD_MATRIX_STATUS_GET_NEW_BLOCK;
                memset (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[0],-1,STICK20_PASSWORD_MATRIX_DATA_LEN);
                break;
            case 2 :
                DebugAppendTextGui ("GetStick20PasswordMatrixData 2\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[0],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 3 :
                DebugAppendTextGui ("GetStick20PasswordMatrixData 3\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[20],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 4 :
                DebugAppendTextGui ("GetStick20PasswordMatrixData 4\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[40],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 5 :
                DebugAppendTextGui ("GetStick20PasswordMatrixData 5\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[60],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 6 :
                DebugAppendTextGui ("GetStick20PasswordMatrixData 6\n");
                memcpy (&HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[80],
                        &HID_Stick20ReceiveData_st.SendData_u8[1],
                        20);
                break;
            case 7 :
                DebugAppendTextGui ("GetStick20PasswordMatrixData 7 - All in\n");
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
        if (OUTPUT_CMD_STICK20_SEND_DATA_SIZE <= len)
        {
            len = OUTPUT_CMD_STICK20_SEND_DATA_SIZE-1;
        }
        HID_Stick20ReceiveData_st.SendData_u8[len] = 0;
        
        
        DebugAppendTextStick ((char *)&HID_Stick20ReceiveData_st.SendData_u8[0]);

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

#ifdef Q_OS_MAC
//    To Check ....
    memcpy ((void*)&HID_Stick20ReceiveData_st,data+1+OUTPUT_CMD_RESULT_STICK20_DATA_START,sizeof (HID_Stick20ReceiveData_st));
#endif

/*
{
    char text[1000];
    int i;

    if (OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG == HID_Stick20ReceiveData_st.SendDataType_u8)
    {
        SNPRINTF(text,sizeof (text),"<%d>",HID_Stick20ReceiveData_st.SendCounter_u8);
        DebugAppendTextGui (text);
    }
    else
    {
        SNPRINTF(text,sizeof (text),"-%d-",HID_Stick20ReceiveData_st.SendCounter_u8);
        DebugAppendTextGui (text);
    }
}
*/
if ((OUTPUT_CMD_STICK20_SEND_DATA_TYPE_NONE  != HID_Stick20ReceiveData_st.SendDataType_u8) &&
    (OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG != HID_Stick20ReceiveData_st.SendDataType_u8))         // Don't log debug data
{
        char text[1000];
        int i;

        SNPRINTF(text,sizeof (text),"HID_GetStick20ReceiveData: ");

        DebugAppendTextGui (text);
        for (i=0;i<64;i++)
        {
            SNPRINTF(text,sizeof (text),"%02x ",data[i]);
            DebugAppendTextGui (text);
        }
        SNPRINTF(text,sizeof (text),"\n");
        DebugAppendTextGui (text);


        SNPRINTF (text,sizeof (text),"HID_GetStick20ReceiveData: SendCounter %d Typ %d - %d - Size %d\n",
                    HID_Stick20ReceiveData_st.SendCounter_u8,
                    HID_Stick20ReceiveData_st.SendDataType_u8,
                    HID_Stick20ReceiveData_st.FollowBytesFlag_u8,
                    HID_Stick20ReceiveData_st.SendSize_u8
                );

        DebugAppendTextGui (text);

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

