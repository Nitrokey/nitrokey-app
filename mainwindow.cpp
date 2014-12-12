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
#include <string.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "device.h"
#include "response.h"
#include "string.h"
#include "sleep.h"
#include "base32.h"
#include "passworddialog.h"
#include "pindialog.h"
#include "hotpdialog.h"
#include "stick20dialog.h"
#include "stick20debugdialog.h"
#include "aboutdialog.h"

#include "device.h"
#include "response.h"
#include "stick20responsedialog.h"
#include "stick20matrixpassworddialog.h"
#include "stick20setup.h"
#include "stick20updatedialog.h"
#include "stick20changepassworddialog.h"
#include "stick20infodialog.h"
#include "stick20hiddenvolumedialog.h"
#include "stick20lockfirmwaredialog.h"
#include "passwordsafedialog.h"
#include "securitydialog.h"
#include "mcvs-wrapper.h"
#include "cryptostick-applet.h"

#include <QTimer>
#include <QMenu>
#include <QDialog>
#include <QtWidgets>
#include <QDateTime>
#include <QThread>

/*******************************************************************************

 External declarations

*******************************************************************************/

//extern "C" char DebugText_Stick20[600000];

extern "C" void DebugAppendTextGui (char *Text);
extern "C" void DebugInitDebugging (void);

/*******************************************************************************

 Local defines

*******************************************************************************/

class OwnSleep : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep (unsigned long secs) {QThread::sleep(secs);}
};

#define LOCAL_PASSWORD_SIZE         40


/*******************************************************************************

  MainWindow

  Constructor MainWindow

  Init the debug output dialog

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

MainWindow::MainWindow(StartUpParameter_tst *StartupInfo_st,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //Q_INIT_RESOURCE(stylesheet);

    int ret;
    QMetaObject::Connection ret_connection;

    HOTP_SlotCount = HOTP_SLOT_COUNT;
    TOTP_SlotCount = TOTP_SLOT_COUNT;

    trayMenu               = NULL;
    Stick20ScSdCardOnline          = FALSE;
    CryptedVolumeActive    = FALSE;
    HiddenVolumeActive     = FALSE;
    NormalVolumeRWActive   = FALSE;
    HiddenVolumeAccessable = FALSE;
    StickNotInitated       = FALSE;
    SdCardNotErased        = FALSE;
    MatrixInputActive      = FALSE;
    LockHardware           = FALSE;

    SdCardNotErased_DontAsk   = FALSE;
    StickNotInitated_DontAsk  = FALSE;

    PWS_Access       = FALSE;
    PWS_CreatePWSize = 12;


    clipboard = QApplication::clipboard();  
  
    ExtendedConfigActive = StartupInfo_st->ExtendedConfigActive;

    if (0 != StartupInfo_st->PasswordMatrix)
    {
        MatrixInputActive = TRUE;
    }

    if (0 != StartupInfo_st->LockHardware)
    {
        LockHardware = TRUE;
    }


    switch (StartupInfo_st->FlagDebug)
    {
        case DEBUG_STATUS_LOCAL_DEBUG :
            DebugWindowActive            = TRUE;
            DebugingActive               = TRUE;
            DebugingStick20PoolingActive = FALSE;
            break;

        case DEBUG_STATUS_DEBUG_ALL :
            DebugWindowActive            = TRUE;
            DebugingActive               = TRUE;
            DebugingStick20PoolingActive = TRUE;
            break;

        case DEBUG_STATUS_NO_DEBUGGING :
        default :
            DebugWindowActive            = FALSE;
            DebugingActive               = FALSE;
            DebugingStick20PoolingActive = FALSE;
            break;
    }


    ui->setupUi(this);

    ui->tabWidget->setCurrentIndex (0); // Set first tab active

    validator = new QIntValidator(0, 9999999, this);
    ui->counterEdit->setValidator(validator);

//    ui->PWS_ButtonCreatePW->setText(QString("Generate random password ").append(QString::number(PWS_CreatePWSize,10).append(QString(" chars"))));
    ui->PWS_ButtonCreatePW->setText(QString("Generate random password "));

    ui->statusBar->showMessage("Device disconnected.");

    cryptostick =  new Device(VID_STICK_OTP, PID_STICK_OTP,VID_STICK_20,PID_STICK_20,VID_STICK_20_UPDATE_MODE,PID_STICK_20_UPDATE_MODE);

// Check for comamd line execution after init "cryptostick"
    if (0 != StartupInfo_st->Cmd)
    {
        ret = ExecStickCmd (StartupInfo_st->CmdLine);
        exit (ret);
    }

    set_initial_time = false;
    QTimer *timer = new QTimer(this);
    ret_connection = connect(timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
    timer->start(1000);


    QTimer *Clipboard_ValidTimer = new QTimer(this);

    // Start timer for Clipboard delete check
    connect(Clipboard_ValidTimer, SIGNAL(timeout()), this, SLOT(checkClipboard_Valid()));
    Clipboard_ValidTimer->start(1000);

    QTimer *Password_ValidTimer = new QTimer(this);

    // Start timer for Password check
    connect(Password_ValidTimer, SIGNAL(timeout()), this, SLOT(checkPasswordTime_Valid()));
    Password_ValidTimer->start(1000);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/images/CS_icon.png"));

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    trayIcon->show();

    if (TRUE == trayIcon->supportsMessages ())
    {
        if (TRUE == DebugWindowActive)
        {
            trayIcon->showMessage ("Cryptostick App","active - DEBUG Mode", QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
        }
        else
        {
            trayIcon->showMessage ("Cryptostick App","active",QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
        }
    }
    else
    {
        if (TRUE == DebugWindowActive)
        {
            csApplet->messageBox("Crypto Stick App is active in DEBUG Mode");
        }
        else
        {
            csApplet->messageBox("Crypto Stick App is active");
        }
    }

    if (FALSE == DebugWindowActive)
    {
        ui->frame_6->setVisible(false);
        ui->testHOTPButton->setVisible(false);
        ui->testTOTPButton->setVisible(false);
        ui->testsSpinBox->setVisible(false);
        ui->testsSpinBox_2->setVisible(false);
        ui->testsLabel->setVisible(false);
        ui->testsLabel_2->setVisible(false);
    }

    quitAction = new QAction(tr("&Quit"), this);
    quitAction->setIcon(QIcon(":/images/quit.png"));
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));


    UnlockPasswordSafe = new QAction("Unlock password safe", this);
    UnlockPasswordSafe->setIcon(QIcon(":/images/safe.png"));
    connect(UnlockPasswordSafe, SIGNAL(triggered()), this, SLOT(PWS_Clicked_EnablePWSAccess()));

    restoreAction = new QAction(tr("&Configure OTP"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(startConfiguration()));

    restoreActionStick20 = new QAction(tr("&Configure OTP and password safe"), this);
    connect(restoreActionStick20, SIGNAL(triggered()), this, SLOT(startConfiguration()));

    DebugAction = new QAction(tr("&Debug"), this);
    connect(DebugAction, SIGNAL(triggered()), this, SLOT(startStickDebug()));

    ActionAboutDialog = new QAction(tr("&About Crypto Stick"), this);
    ActionAboutDialog->setIcon(QIcon(":/images/about.png"));
    connect(ActionAboutDialog,  	 SIGNAL(triggered()), this, SLOT(startAboutDialog()));

    initActionsForStick20 ();

    connect(ui->secretEdit, SIGNAL(textEdited(QString)), this, SLOT(checkTextEdited()));

    // Init debug text
    DebugInitDebugging ();
    DebugAppendTextGui ((char *)"Start Debug - ");

#ifdef WIN32
    DebugAppendTextGui ((char *)"WIN32 system\n");
#endif

#ifdef linux
    DebugAppendTextGui ((char *)"LINUX system\n");
#endif

#ifdef MAC
    DebugAppendTextGui ((char *)"MAC system\n");
#endif

    {
        union {
            unsigned char input[8];
            unsigned int  endianCheck[2];
            unsigned long long endianCheck_ll;
        } uEndianCheck;

        char text[50];

        DebugAppendTextGui ((char *)"\nEndian check\n\n");

        DebugAppendTextGui ((char *)"Store 0x01 0x02 0x03 0x04 in memory locations x,x+1,x+2,x+3\n");
        DebugAppendTextGui ((char *)"then read the location x - x+3 as an unsigned int\n\n");

        uEndianCheck.input[0] = 0x01;
        uEndianCheck.input[1] = 0x02;
        uEndianCheck.input[2] = 0x03;
        uEndianCheck.input[3] = 0x04;
        uEndianCheck.input[4] = 0x05;
        uEndianCheck.input[5] = 0x06;
        uEndianCheck.input[6] = 0x07;
        uEndianCheck.input[7] = 0x08;

        SNPRINTF(text,sizeof (text),"write u8  %02x%02x%02x%02x%02x%02x%02x%02x\n",uEndianCheck.input[0],uEndianCheck.input[1],uEndianCheck.input[2],uEndianCheck.input[3],uEndianCheck.input[4],uEndianCheck.input[5],uEndianCheck.input[6],uEndianCheck.input[7]);
        DebugAppendTextGui (text);

        SNPRINTF(text,sizeof (text),"read  u32 0x%08x  u32 0x%08x\n",uEndianCheck.endianCheck[0],uEndianCheck.endianCheck[1]);
        DebugAppendTextGui (text);

        SNPRINTF(text,sizeof (text),"read  u64 0x%08x%08x\n",(unsigned long)(uEndianCheck.endianCheck_ll>>32),(unsigned long)uEndianCheck.endianCheck_ll);
        DebugAppendTextGui (text);

        DebugAppendTextGui ("\n");

        if (0x01020304 == uEndianCheck.endianCheck[0])
        {
            DebugAppendTextGui ("System is little endian\n");
        }
        if (0x04030201 == uEndianCheck.endianCheck[0])
        {
            DebugAppendTextGui ("System is big endian\n");
        }
        DebugAppendTextGui ("\n");

        DebugAppendTextGui ("Var size test\n");

        SNPRINTF(text,sizeof (text),"char  size is %d byte\n",sizeof (unsigned char));
        DebugAppendTextGui (text);

        SNPRINTF(text,sizeof (text),"short size is %d byte\n",sizeof (unsigned short));
        DebugAppendTextGui (text);

        SNPRINTF(text,sizeof (text),"int   size is %d byte\n",sizeof (unsigned int));
        DebugAppendTextGui (text);

        SNPRINTF(text,sizeof (text),"long  size is %d byte\n",sizeof (unsigned long));
        DebugAppendTextGui (text);
        DebugAppendTextGui ("\n");
    }

    //ui->labelQuestion1->setToolTip("Test");
    ui->deleteUserPasswordCheckBox->setEnabled(false);
    ui->deleteUserPasswordCheckBox->setChecked(false);

    cryptostick->getStatus();

    generateMenu();

}


/*******************************************************************************

  ExecStickCmd

  Changes
  Date      Author        Info
  03.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/
#define MAX_CONNECT_WAIT_TIME_IN_SEC       10

int MainWindow::ExecStickCmd(char *Cmdline)
{
    int i;
    char *p;
    bool ret;

    printf ("Connect to crypto stick\n");

// Wait for connect
    for (i=0;i<MAX_CONNECT_WAIT_TIME_IN_SEC;i++)
    {
        if (cryptostick->isConnected == true)
        {
            break;
        }
        else
        {
            cryptostick->connect();
            OwnSleep::sleep (1);
            cryptostick->checkConnection();
        }
    }
    if (MAX_CONNECT_WAIT_TIME_IN_SEC <= i)
    {
        printf ("ERROR: Can't get connection to crypto stick\n");
        return (1);
    }
// Check device
    printf ("Get connection to crypto stick\n");

// Get command
    p = strchr (Cmdline,'=');
    if (NULL == p)
    {
        p = NULL;
    }
    else
    {
        *p = 0;
        p++;  // Points now to 1. parameter
    }

    if (0 == strcmp (Cmdline,"unlockencrypted"))
    {
        uint8_t password[40];

        STRCPY ((char*)password,sizeof (password),"P123456");
        printf ("Unlock encrypted volume: ");

        ret = cryptostick->stick20EnableCryptedPartition (password);

        if (false == ret)
        {
            printf ("FAIL sending command via HID\n");
            return (1);
        }
    }

    if (0 == strcmp (Cmdline,"prodinfo"))
    {
        printf ("Send -get production infos-\n");

        stick20SendCommand (STICK20_CMD_PRODUCTION_TEST,NULL);

        ret = cryptostick->stick20ProductionTest();

/*
        Stick20ResponseDialog ResponseDialog(this);
        ResponseDialog.NoStopWhenStatusOK ();
        ResponseDialog.cryptostick=cryptostick;
        ResponseDialog.exec();
        ret = ResponseDialog.ResultValue;
*/

        if (false == ret)
        {
            printf ("FAIL sending command via HID\n");
            return (1);
        }

        if (TRUE == Stick20_ProductionInfosChanged)
        {
            Stick20_ProductionInfosChanged = FALSE;
            AnalyseProductionInfos ();
        }

    }
    return (0);
}

/*******************************************************************************

  iconActivated

  Changes
  Date      Author        Info
  31.01.14  RB            Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{

    switch (reason) {
    case QSystemTrayIcon::Context:
//        trayMenu->hide();
//        trayMenu->close();
#ifdef Q_OS_MAC
        trayMenu->popup(QCursor::pos());
#endif
        break;
    case QSystemTrayIcon::Trigger:
#ifndef Q_OS_MAC
        trayMenu->popup(QCursor::pos());
#endif
        break;
    case QSystemTrayIcon::DoubleClick:
        break;
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        ;
    }
}

/*******************************************************************************

  eventFilter

  Changes
  Date      Author        Info
  31.01.14  RB            Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

bool MainWindow::eventFilter (QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
      QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
      if(mEvent->button() == Qt::LeftButton)
      {
/*
        QMouseEvent my_event = new QMouseEvent ( mEvent->type(), mEvent->pos(), Qt::Rightbutton ,
        mEvent->buttons(), mEvent->modifiers() );
        QCoreApplication::postEvent ( trayIcon, my_event );
*/
        return true;
      }
    }
    return QObject::eventFilter(obj, event);
}

/*******************************************************************************

  AnalyseProductionInfos

  Changes
  Date      Author        Info
  07.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::AnalyseProductionInfos()
{
    char text[100];
    bool ProductStateOK = TRUE;

    printf ("\nGet production infos\n");

    printf ("Firmware     %d.%d\n",Stick20ProductionInfos_st.FirmwareVersion_au8[0],Stick20ProductionInfos_st.FirmwareVersion_au8[1]);
    printf ("CPU ID       0x%08lx\n",Stick20ProductionInfos_st.CPU_CardID_u32);
    printf ("Smartcard ID 0x%08lx\n",Stick20ProductionInfos_st.SmartCardID_u32);
    printf ("SD card ID   0x%08lx\n",Stick20ProductionInfos_st.SD_CardID_u32);

    printf ((char*)"\nSD card infos\n");
    printf ("Manufacturer 0x%02x\n",Stick20ProductionInfos_st.SD_Card_Manufacturer_u8);
    printf ("OEM          0x%04x\n",Stick20ProductionInfos_st.SD_Card_OEM_u16);
    printf ("Manufa. date %d.%02d\n",Stick20ProductionInfos_st.SD_Card_ManufacturingYear_u8+2000,Stick20ProductionInfos_st.SD_Card_ManufacturingMonth_u8);
    printf ("Write speed  %d kB/sec\n",Stick20ProductionInfos_st.SD_WriteSpeed_u16);

// Check for SD card speed
    if (5000 > Stick20ProductionInfos_st.SD_WriteSpeed_u16)
    {
       ProductStateOK = FALSE;
       printf ((char*)"Speed error\n");
    }

// Valid manufacturers
    if (    (0x74 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8)     // Transcend
            && (0x03 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8)     // SanDisk
            && (0x73 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8)     // ? Amazon
            && (0x28 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8)     // Lexar
            && (0x1b != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8)     // Samsung
       )
    {
        ProductStateOK = FALSE;
        printf ((char*)"Manufacturers error\n");
    }

// Write to log
    {
        FILE *fp;
        char *LogFile = (char *)"prodlog.txt";

        FOPEN( fp, LogFile,"a+");
        if (0 != fp)
        {
            fprintf (fp,"CPU:0x%08lx,",Stick20ProductionInfos_st.CPU_CardID_u32);
            fprintf (fp,"SC:0x%08lx,",Stick20ProductionInfos_st.SmartCardID_u32);
            fprintf (fp,"SD:0x%08lx,",Stick20ProductionInfos_st.SD_CardID_u32);
            fprintf (fp,"SCM:0x%02x,",Stick20ProductionInfos_st.SD_Card_Manufacturer_u8);
            fprintf (fp,"SCO:0x%04x,",Stick20ProductionInfos_st.SD_Card_OEM_u16);
            fprintf (fp,"DAT:%d.%02d,",Stick20ProductionInfos_st.SD_Card_ManufacturingYear_u8+2000,Stick20ProductionInfos_st.SD_Card_ManufacturingMonth_u8);
            fprintf (fp,"Speed:%d",Stick20ProductionInfos_st.SD_WriteSpeed_u16);
            if (FALSE == ProductStateOK)
            {
                fprintf (fp,",*** FAIL");
            }
            fprintf (fp,"\n");
            fclose (fp);
        }
        else
        {
            printf ((char*)"\n*** CAN'T WRITE TO LOG FILE -%s-***\n",LogFile);
        }
    }

    if (TRUE == ProductStateOK)
    {
        printf ((char*)"\nStick OK\n");
    }
    else
    {
        printf ((char*)"\n**** Stick NOT OK ****\n");
    }

    DebugAppendTextGui ((char *)"Production Infos\n");

    SNPRINTF(text,sizeof (text),"Firmware     %d.%d\n",Stick20ProductionInfos_st.FirmwareVersion_au8[0],Stick20ProductionInfos_st.FirmwareVersion_au8[1]);
    DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"CPU ID       0x%08lx\n",Stick20ProductionInfos_st.CPU_CardID_u32);
    DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"Smartcard ID 0x%08lx\n",Stick20ProductionInfos_st.SmartCardID_u32);
    DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"SD card ID   0x%08lx\n",Stick20ProductionInfos_st.SD_CardID_u32);
    DebugAppendTextGui (text);


    DebugAppendTextGui ("Password retry count\n");
    SNPRINTF(text,sizeof (text),"Admin        %d\n",Stick20ProductionInfos_st.SC_AdminPwRetryCount);
    DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"User         %d\n",Stick20ProductionInfos_st.SC_UserPwRetryCount);
    DebugAppendTextGui (text);

    DebugAppendTextGui ("SD card infos\n");
    SNPRINTF(text,sizeof (text),"Manufacturer 0x%02x\n",Stick20ProductionInfos_st.SD_Card_Manufacturer_u8);
    DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"OEM          0x%04x\n",Stick20ProductionInfos_st.SD_Card_OEM_u16);
    DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"Manufa. date %d.%02d\n",Stick20ProductionInfos_st.SD_Card_ManufacturingYear_u8+2000,Stick20ProductionInfos_st.SD_Card_ManufacturingMonth_u8);
    DebugAppendTextGui (text);
    SNPRINTF(text,sizeof (text),"Write speed  %d kB/sec\n",Stick20ProductionInfos_st.SD_WriteSpeed_u16);
    DebugAppendTextGui (text);
}

/*******************************************************************************

  checkConnection

  Changes
  Date      Author        Info
  07.07.14  RB            Implementation production infos

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::checkConnection()
{
    //static int Stick20CheckFlashMode = TRUE;
    static int DeviceOffline         = TRUE;
    int ret;

    currentTime = QDateTime::currentDateTime().toTime_t();

    int result = cryptostick->checkConnection();

// Set new slot counts
    HOTP_SlotCount = cryptostick->HOTP_SlotCount;
    TOTP_SlotCount = cryptostick->TOTP_SlotCount;

    if (result==0)
    {
        if (false == cryptostick->activStick20)
        {
            ui->statusBar->showMessage("Cryptostick Classic connected.");

            if(set_initial_time == FALSE) {
                ret = cryptostick->setTime(TOTP_CHECK_TIME);
                set_initial_time = TRUE;
            } else {
                ret = 0;
            }

            bool answer;
            if(ret == -2){
                 answer = csApplet->detailedYesOrNoBox("Time is out-of-sync",
                 "WARNING!\n\nThe time of your computer and Crypto Stick are out of sync. Your computer may be configured with a wrong time or your Crypto Stick may have been attacked. If an attacker or malware could have used your Crypto Stick you should reset the secrets of your configured One Time Passwords. If your computer's time is wrong, please configure it correctly and reset the time of your Crypto Stick.\n\nReset Crypto Stick's time?",
                 0, false);
                 if (answer) {
                        resetTime();
                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        Sleep::msleep(1000);
                        QApplication::restoreOverrideCursor();
                        generateAllConfigs();

                        csApplet->messageBox("Time reset!");
                 }
             }

            cryptostick->getStatus();
        } else
        {
            ui->statusBar->showMessage("Crypto Stick Storage connected.");
        }
        DeviceOffline = FALSE;
    }
    else if (result == -1)
    {
        ui->statusBar->showMessage("Device disconnected.");
        HID_Stick20Init ();             // Clear stick 20 data
        Stick20ScSdCardOnline = FALSE;
        CryptedVolumeActive   = FALSE;
        HiddenVolumeActive    = FALSE;
        set_initial_time      = FALSE;
        if (FALSE== DeviceOffline)      // To avoid the continuous reseting of the menu
        {
            generateMenu();
            DeviceOffline = TRUE;
            cryptostick->passwordSafeAvailable= true;
            trayIcon->showMessage("Device disconnected.", "", QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
        }
        cryptostick->connect();



/* Don't work :-(
        // Check for stick 20 flash mode, only one time
        if (TRUE == Stick20CheckFlashMode)
        {
            ret = cryptostick->checkUsbDeviceActive (VID_STICK_20_UPDATE_MODE,PID_STICK_20_UPDATE_MODE);
 //           Stick20CheckFlashMode = FALSE;
            if (TRUE == ret)
            {
                QMessageBox msgBox;
                msgBox.setText("Flash Crypto Stick application");
                msgBox.exec();
            }
        }
*/
    }
    else if (result == 1){ //recreate the settings and menus
        if (false == cryptostick->activStick20)
        {
            ui->statusBar->showMessage("Device connected.");
            trayIcon->showMessage("Device connected", "Crypto Stick Classic", QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);

            if(set_initial_time == FALSE){
                ret = cryptostick->setTime(TOTP_CHECK_TIME);
                set_initial_time = TRUE;
            } else {
                ret = 0;
            }

            bool answer;
            if(ret == -2){
                 answer = csApplet->detailedYesOrNoBox("Time is out-of-sync",
                 "WARNING!\n\nThe time of your computer and Crypto Stick are out of sync. Your computer may be configured with a wrong time or your Crypto Stick may have been attacked. If an attacker or malware could have used your Crypto Stick you should reset the secrets of your configured One Time Passwords. If your computer's time is wrong, please configure it correctly and reset the time of your Crypto Stick.\n\nReset Crypto Stick's time?",
                 0, false);

                 if (answer)
                 {
                        resetTime();
                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        Sleep::msleep(1000);
                        QApplication::restoreOverrideCursor();
                        generateAllConfigs();

                        csApplet->messageBox("Time reset!");
                 }
             }

            cryptostick->getStatus();
        } else
        {
            trayIcon->showMessage("Device connected", "Crypto Stick Storage", QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
            ui->statusBar->showMessage("Crypto Stick Storage connected.");
        }
        generateMenu();
    }

// Be sure that the retry counter are always up to date
    if ( (cryptostick->userPasswordRetryCount != HID_Stick20Configuration_st.UserPwRetryCount)) // (99 != HID_Stick20Configuration_st.UserPwRetryCount) &&
    {
        cryptostick->userPasswordRetryCount = HID_Stick20Configuration_st.UserPwRetryCount;
        cryptostick->passwordRetryCount     = HID_Stick20Configuration_st.AdminPwRetryCount;
    }

    if (TRUE == Stick20_ConfigurationChanged)
    {
        Stick20_ConfigurationChanged = FALSE;

        UpdateDynamicMenuEntrys ();


        if (TRUE == StickNotInitated)
        {

            if (FALSE == StickNotInitated_DontAsk)
            {
                csApplet->warningBox("Warning: Encrypted volume is not secure.\nSelect \"Initialize keys\"");
            }
        }
        if (TRUE == SdCardNotErased)
        {
            if (FALSE == SdCardNotErased_DontAsk)
            {
                csApplet->warningBox("Warning: Encrypted volume is not secure,\nSelect \"Initialize storage with random data\"");
            }
        }

    }
    if (TRUE == Stick20_ProductionInfosChanged)
    {
        Stick20_ProductionInfosChanged = FALSE;

        AnalyseProductionInfos ();
    }
    if(ret){}//Fix warnings
}

/*******************************************************************************

  startTimer

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::startTimer()
{
}

/*******************************************************************************

  ~MainWindow

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

MainWindow::~MainWindow()
{
    delete validator;
    delete ui;
}

/*******************************************************************************

  closeEvent

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}

/*******************************************************************************

  on_pushButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/
void MainWindow::on_pushButton_clicked()
{
    if (cryptostick->isConnected){
        int64_t crc = cryptostick->getSlotName(0x11);

        Sleep::msleep(100);
        Response *testResponse=new Response();
        testResponse->getResponse(cryptostick);

        if (crc==testResponse->lastCommandCRC){

            QMessageBox message;
            QString str;
            //str.append(QString::number(testResponse->lastCommandCRC,16));
            QByteArray *data =new QByteArray(testResponse->data);
            str.append(QString(data->toHex()));

            message.setText(str);
            message.exec();

        }
    }
}
/*******************************************************************************

  on_pushButton_2_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/
/*
void MainWindow::on_pushButton_2_clicked()
{
}
*/
/*******************************************************************************

  getSlotNames

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::getSlotNames()
{
}

/*******************************************************************************

  generateComboBoxEntrys

  Changes
  Date      Author        Info
  07.05.14  RB            Function created

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

void MainWindow::generateComboBoxEntrys()
{
    int i;

    ui->slotComboBox->clear();


    for (i=0;i<TOTP_SlotCount;i++)
    {
        if ((char)cryptostick->TOTPSlots[i]->slotName[0] == '\0') {
            ui->slotComboBox->addItem(QString("TOTP slot ").append(QString::number(i+1,10)));
        } else {
            ui->slotComboBox->addItem(QString("TOTP slot ").append(QString::number(i+1,10)).append(" [").append((char *)cryptostick->TOTPSlots[i]->slotName).append("]"));
        }
    }

    ui->slotComboBox->insertSeparator(TOTP_SlotCount+1);

    for (i=0;i<HOTP_SlotCount;i++)
    {
        if ((char)cryptostick->HOTPSlots[i]->slotName[0] == '\0') {
            ui->slotComboBox->addItem(QString("HOTP slot ").append(QString::number(i+1,10)));
        } else {
            ui->slotComboBox->addItem(QString("HOTP slot ").append(QString::number(i+1,10)).append(" [").append((char *)cryptostick->HOTPSlots[i]->slotName).append("]"));
        }
    }

    ui->slotComboBox->setCurrentIndex(0);

//    i = ui->slotComboBox->currentIndex();
}

/*******************************************************************************

  generateMenu


  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::generateMenu()
{
    if (NULL == trayMenu)
    {
        trayMenu = new QMenu();
    }
    else
    {
        trayMenu->clear();      // Clear old menu
    }

// Setup the new menu
    if (cryptostick->isConnected == false){
        trayMenu->addAction("Crypto Stick not connected");
        cryptostick->passwordSafeAvailable = true;
    }
    else{
        if (false == cryptostick->activStick20)
        {
            // Stick 10 is connected
            generateMenuForStick10 ();
        }
        else {
            // Stick 20 is connected
            generateMenuForStick20 ();
            cryptostick->passwordSafeAvailable = true;
        }
    }

// Add debug window ?
    if (TRUE == DebugWindowActive)
    {
        trayMenu->addAction(DebugAction);
    }

    trayMenu->addSeparator();

// About entry
    trayMenu->addAction(ActionAboutDialog);

    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);

    generateComboBoxEntrys();

}


/*******************************************************************************

  initActionsForStick20

  Changes
  Date      Author        Info
  03.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::initActionsForStick20()
{
    SecPasswordAction = new QAction(tr("&SecPassword"), this);
    connect(SecPasswordAction, SIGNAL(triggered()), this, SLOT(startMatrixPasswordDialog()));

    Stick20Action = new QAction(tr("&Stick 20"), this);
    connect(Stick20Action, SIGNAL(triggered()), this, SLOT(startStick20Configuration()));

    Stick20SetupAction = new QAction(tr("&Stick 20 Setup"), this);
    connect(Stick20SetupAction, SIGNAL(triggered()), this, SLOT(startStick20Setup()));

    Stick20ActionEnableCryptedVolume = new QAction(tr("&Unlock encrypted volume"), this);
    Stick20ActionEnableCryptedVolume->setIcon(QIcon(":/images/harddrive.png"));
    connect(Stick20ActionEnableCryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20EnableCryptedVolume()));

    Stick20ActionDisableCryptedVolume = new QAction(tr("&Lock encrypted volume"), this);
    Stick20ActionDisableCryptedVolume->setIcon(QIcon(":/images/harddrive.png"));
    connect(Stick20ActionDisableCryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20DisableCryptedVolume()));

    Stick20ActionEnableHiddenVolume = new QAction(tr("&Unlock hidden volume"), this);
    Stick20ActionEnableHiddenVolume->setIcon(QIcon(":/images/harddrive.png"));
    connect(Stick20ActionEnableHiddenVolume, SIGNAL(triggered()), this, SLOT(startStick20EnableHiddenVolume()));

    Stick20ActionDisableHiddenVolume = new QAction(tr("&Lock hidden volume"), this);
    connect(Stick20ActionDisableHiddenVolume, SIGNAL(triggered()), this, SLOT(startStick20DisableHiddenVolume()));

    Stick20ActionChangeUserPIN = new QAction(tr("&Change user PIN"), this);
    connect(Stick20ActionChangeUserPIN, SIGNAL(triggered()), this, SLOT(startStick20ActionChangeUserPIN()));

    Stick20ActionChangeAdminPIN = new QAction(tr("&Change admin PIN"), this);
    connect(Stick20ActionChangeAdminPIN, SIGNAL(triggered()), this, SLOT(startStick20ActionChangeAdminPIN()));

    Stick20ActionEnableFirmwareUpdate = new QAction(tr("&Enable firmware update"), this);
    connect(Stick20ActionEnableFirmwareUpdate, SIGNAL(triggered()), this, SLOT(startStick20EnableFirmwareUpdate()));

    Stick20ActionExportFirmwareToFile = new QAction(tr("&Export firmware to file"), this);
    connect(Stick20ActionExportFirmwareToFile, SIGNAL(triggered()), this, SLOT(startStick20ExportFirmwareToFile()));

    Stick20ActionDestroyCryptedVolume = new QAction(tr("&Destroy encrypted data"), this);
    connect(Stick20ActionDestroyCryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20DestroyCryptedVolume()));

    Stick20ActionInitCryptedVolume = new QAction(tr("&Initialize keys"), this);
    connect(Stick20ActionInitCryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20DestroyCryptedVolume()));

    Stick20ActionFillSDCardWithRandomChars = new QAction(tr("&Initialize storage with random data"), this);
    connect(Stick20ActionFillSDCardWithRandomChars, SIGNAL(triggered()), this, SLOT(startStick20FillSDCardWithRandomChars()));

    Stick20ActionGetStickStatus = new QAction(tr("&Get stick status"), this);
    connect(Stick20ActionGetStickStatus, SIGNAL(triggered()), this, SLOT(startStick20GetStickStatus()));
//    connect(Stick20ActionGetStickStatus, SIGNAL(triggered()), this, SLOT(startAboutDialog()));

    Stick20ActionSetReadonlyUncryptedVolume = new QAction(tr("&Set unencrypted volume read-only"), this);
    connect(Stick20ActionSetReadonlyUncryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20SetReadonlyUncryptedVolume()));

    Stick20ActionSetReadWriteUncryptedVolume = new QAction(tr("&Set unencrypted volume read-write"), this);
    connect(Stick20ActionSetReadWriteUncryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20SetReadWriteUncryptedVolume()));

    Stick20ActionDebugAction = new QAction(tr("&Debug Action"), this);
    connect(Stick20ActionDebugAction, SIGNAL(triggered()), this, SLOT(startStick20DebugAction()));

    Stick20ActionSetupHiddenVolume = new QAction(tr("&Setup hidden volume"), this);
    connect(Stick20ActionSetupHiddenVolume, SIGNAL(triggered()), this, SLOT(startStick20SetupHiddenVolume()));

    Stick20ActionClearNewSDCardFound = new QAction(tr("&Disable 'initialize storage with random data' warning"), this);
    connect(Stick20ActionClearNewSDCardFound, SIGNAL(triggered()), this, SLOT(startStick20ClearNewSdCardFound()));

    Stick20ActionSetupPasswordMatrix = new QAction(tr("&Setup password matrix"), this);
    connect(Stick20ActionSetupPasswordMatrix, SIGNAL(triggered()), this, SLOT(startStick20SetupPasswordMatrix()));

    Stick20ActionLockStickHardware = new QAction(tr("&Lock stick hardware"), this);
    connect(Stick20ActionLockStickHardware, SIGNAL(triggered()), this, SLOT(startStick20LockStickHardware()));

    Stick20ActionResetUserPassword = new QAction(tr("&Reset user PIN"), this);
    connect(Stick20ActionResetUserPassword, SIGNAL(triggered()), this, SLOT(startResetUserPassword()));

    LockDeviceAction = new QAction(tr("&Lock Device"), this);
    connect(LockDeviceAction, SIGNAL(triggered()), this, SLOT(startLockDeviceAction()));

    Stick20ActionUpdateStickStatus = new QAction(tr("Smartcard or SD card are not ready"), this);
    connect(Stick20ActionUpdateStickStatus, SIGNAL(triggered()), this, SLOT(startAboutDialog()));
}

/*******************************************************************************

  generatePasswordMenu

  Changes
  Date      Author        Info
  24.03.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::generatePasswordMenu()
{
    int i;
    
    trayMenuPasswdSubMenu = trayMenu->addMenu("Passwords");

    /* TOTP passwords */
    if (cryptostick->TOTPSlots[0]->isProgrammed==true){
        trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[0]->slotName, this,SLOT(getTOTP1()));
    }
    if (cryptostick->TOTPSlots[1]->isProgrammed==true){
        trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[1]->slotName, this,SLOT(getTOTP2()));
    }
    if (cryptostick->TOTPSlots[2]->isProgrammed==true){
        trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[2]->slotName, this,SLOT(getTOTP3()));
    }

    if (cryptostick->TOTPSlots[3]->isProgrammed==true){
        trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[3]->slotName, this,SLOT(getTOTP4()));
    }
    if (TOTP_SlotCount > 4)
    {
        if (cryptostick->TOTPSlots[4]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[4]->slotName, this,SLOT(getTOTP5()));
        }
    }

    if (TOTP_SlotCount > 5)
    {
        if (cryptostick->TOTPSlots[5]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[5]->slotName, this,SLOT(getTOTP6()));
        }
    }

    if (TOTP_SlotCount > 6)
    {
        if (cryptostick->TOTPSlots[6]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[6]->slotName, this,SLOT(getTOTP7()));
        }
    }

    if (TOTP_SlotCount > 7)
    {
        if (cryptostick->TOTPSlots[7]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[7]->slotName, this,SLOT(getTOTP8()));
        }
    }

    if (TOTP_SlotCount > 8)
    {
        if (cryptostick->TOTPSlots[8]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[8]->slotName, this,SLOT(getTOTP9()));
        }
    }

    if (TOTP_SlotCount > 9)
    {
        if (cryptostick->TOTPSlots[9]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[8]->slotName, this,SLOT(getTOTP10()));
        }
    }

    if (TOTP_SlotCount > 10)
    {
        if (cryptostick->TOTPSlots[10]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[10]->slotName, this,SLOT(getTOTP11()));
        }
    }

    if (TOTP_SlotCount > 11)
    {
        if (cryptostick->TOTPSlots[11]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[11]->slotName, this,SLOT(getTOTP12()));
        }
    }

    if (TOTP_SlotCount > 12)
    {
        if (cryptostick->TOTPSlots[12]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[12]->slotName, this,SLOT(getTOTP13()));
        }
    }

    if (TOTP_SlotCount > 13)
    {
        if (cryptostick->TOTPSlots[13]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[13]->slotName, this,SLOT(getTOTP14()));
        }
    }

    if (TOTP_SlotCount > 14)
    {
        if (cryptostick->TOTPSlots[14]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[14]->slotName, this,SLOT(getTOTP15()));
        }
    }


    /* HOTP passwords */
    if (cryptostick->HOTPSlots[0]->isProgrammed==true){
        trayMenuPasswdSubMenu->addAction((char *)cryptostick->HOTPSlots[0]->slotName, this,SLOT(getHOTP1()));
    }
    if (cryptostick->HOTPSlots[1]->isProgrammed==true){
        trayMenuPasswdSubMenu->addAction((char *)cryptostick->HOTPSlots[1]->slotName, this,SLOT(getHOTP2()));
    }

    if (HOTP_SlotCount >= 3)
    {
        if (cryptostick->HOTPSlots[2]->isProgrammed==true){
            trayMenuPasswdSubMenu->addAction((char *)cryptostick->HOTPSlots[2]->slotName, this,SLOT(getHOTP3()));
        }
    }



    if (TRUE == cryptostick->passwordSafeUnlocked) 
    {
        if (cryptostick->passwordSafeStatus[0] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (0),this,SLOT(PWS_Clicked_Slot00()));
        }
        if (cryptostick->passwordSafeStatus[1] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (1),this,SLOT(PWS_Clicked_Slot01()));
        }
        if (cryptostick->passwordSafeStatus[2] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (2),this,SLOT(PWS_Clicked_Slot02()));
        }
        if (cryptostick->passwordSafeStatus[3] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (3),this,SLOT(PWS_Clicked_Slot03()));
        }
        if (cryptostick->passwordSafeStatus[4] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (4),this,SLOT(PWS_Clicked_Slot04()));
        }
        if (cryptostick->passwordSafeStatus[5] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (5),this,SLOT(PWS_Clicked_Slot05()));
        }
        if (cryptostick->passwordSafeStatus[6] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (6),this,SLOT(PWS_Clicked_Slot06()));
        }
        if (cryptostick->passwordSafeStatus[7] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (7),this,SLOT(PWS_Clicked_Slot07()));
        }
        if (cryptostick->passwordSafeStatus[8] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (8),this,SLOT(PWS_Clicked_Slot08()));
        }
        if (cryptostick->passwordSafeStatus[9] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (9),this,SLOT(PWS_Clicked_Slot09()));
        }
        if (cryptostick->passwordSafeStatus[10] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (10),this,SLOT(PWS_Clicked_Slot10()));
        }
        if (cryptostick->passwordSafeStatus[11] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (11),this,SLOT(PWS_Clicked_Slot11()));
        }
        if (cryptostick->passwordSafeStatus[12] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (12),this,SLOT(PWS_Clicked_Slot12()));
        }
        if (cryptostick->passwordSafeStatus[13] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (13),this,SLOT(PWS_Clicked_Slot13()));
        }
        if (cryptostick->passwordSafeStatus[14] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (14),this,SLOT(PWS_Clicked_Slot14()));
        }
        if (cryptostick->passwordSafeStatus[15] == (unsigned char)true)
        {
            trayMenuPasswdSubMenu->addAction(PWS_GetSlotName (15),this,SLOT(PWS_Clicked_Slot15()));
        }       
    }

    trayMenu->addSeparator();
}

/*******************************************************************************

  generateMenuForStick10

  Changes
  Date      Author        Info
  27.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::generateMenuForStick10()
{
    // Hide tab for password safe for stick 1.x
//    ui->tabWidget->removeTab(3);        // 3 = ui->tab_3 = password safe

    generatePasswordMenu ();
    trayMenu->addSeparator();

    generateMenuPasswordSafe ();
/*
    if (FALSE == StickNotInitated)
    {
        // Enable tab for password safe for stick 2
        if (-1 == ui->tabWidget->indexOf (ui->tab_3))
        {
            ui->tabWidget->addTab(ui->tab_3,"Password Safe");
        }
        ui->pushButton_StaticPasswords->show ();

        // Setup entrys for password safe
    }

    ui->pushButton_StaticPasswords->hide ();
*/


    if (TRUE == cryptostick->passwordSafeAvailable)
    {    
        trayMenuSubConfigure  = trayMenu->addMenu( "Configure" );
        trayMenuSubConfigure->setIcon(QIcon(":/images/settings.png"));
        trayMenuSubConfigure->addAction(restoreActionStick20);
    }
    else {
        trayMenu->addAction(restoreAction);
        restoreAction->setIcon(QIcon(":/images/settings.png"));
    }

}

/*******************************************************************************

  generateMenuForStick20

  Changes
  Date      Author        Info
  03.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::generateMenuForStick20()
{
    //int i;
    int AddSeperator;

    if (FALSE == Stick20ScSdCardOnline)         // Is Stick 2.0 online (SD + SC accessable?)
    {
        trayMenu->addAction(Stick20ActionUpdateStickStatus);
        return;
    }

// Add special entrys
    AddSeperator = FALSE;

    if (TRUE == StickNotInitated)
    {
        trayMenu->addAction(Stick20ActionInitCryptedVolume       );
        AddSeperator = TRUE;
    }

    if (TRUE == SdCardNotErased)
    {
        trayMenu->addAction(Stick20ActionFillSDCardWithRandomChars  );
        AddSeperator = TRUE;
    }

    if (TRUE == AddSeperator)
    {
        trayMenu->addSeparator();
    }

    generatePasswordMenu ();
    trayMenu->addSeparator();

    if (FALSE == StickNotInitated)
    {
// Enable tab for password safe for stick 2
        if (-1 == ui->tabWidget->indexOf (ui->tab_3))
        {
            ui->tabWidget->addTab(ui->tab_3,"Password Safe");
        }
        ui->pushButton_StaticPasswords->show ();

// Setup entrys for password safe
        generateMenuPasswordSafe ();
    }

    if (FALSE == SdCardNotErased)
    {
        if (FALSE == CryptedVolumeActive)
        {
            trayMenu->addAction(Stick20ActionEnableCryptedVolume        );
        }
        else
        {
            trayMenu->addAction(Stick20ActionDisableCryptedVolume       );
        }


        if (FALSE == HiddenVolumeActive)
        {
            trayMenu->addAction(Stick20ActionEnableHiddenVolume         );
        }
        else
        {
            trayMenu->addAction(Stick20ActionDisableHiddenVolume        );
        }
    }

    trayMenu->addAction(LockDeviceAction);

    trayMenuSubConfigure = trayMenu->addMenu( "Configure" );
    trayMenuSubConfigure->setIcon(QIcon(":/images/settings.png"));
    trayMenuSubConfigure->addAction(restoreActionStick20);
    trayMenuSubConfigure->addAction(Stick20ActionChangeUserPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeAdminPIN);

    if (FALSE == NormalVolumeRWActive)
    {
        trayMenuSubConfigure->addAction(Stick20ActionSetReadonlyUncryptedVolume );      // Set RW active
    }
    else
    {
        trayMenuSubConfigure->addAction(Stick20ActionSetReadWriteUncryptedVolume);      // Set readonly active
    }

    if (FALSE == SdCardNotErased)
    {
        trayMenuSubConfigure->addAction(Stick20ActionSetupHiddenVolume);
    }

    if (TRUE == MatrixInputActive)
    {
        trayMenuSubConfigure->addAction(Stick20ActionSetupPasswordMatrix);
    }

    trayMenuSubConfigure->addAction(Stick20ActionDestroyCryptedVolume);
//    trayMenuSubConfigure->addAction(Stick20ActionGetStickStatus             );


    if (TRUE == LockHardware)
    {
        trayMenuSubConfigure->addAction(Stick20ActionLockStickHardware);
    }

    if (TRUE == HiddenVolumeAccessable)
    {

    }

    trayMenuSubConfigure->addAction(Stick20ActionEnableFirmwareUpdate       );
    trayMenuSubConfigure->addAction(Stick20ActionExportFirmwareToFile       );

    if (TRUE == ExtendedConfigActive)
    {
        trayMenuSubSpecialConfigure = trayMenuSubConfigure->addMenu( "Special Configure" );
        trayMenuSubSpecialConfigure->addAction(Stick20ActionFillSDCardWithRandomChars);

        if (TRUE == SdCardNotErased)
        {
            trayMenuSubSpecialConfigure->addAction(Stick20ActionClearNewSDCardFound);
        }
    }


// Enable "reset user PIN" ?
    if (0 == cryptostick->userPasswordRetryCount)
    {
        trayMenu->addSeparator();
        trayMenu->addAction(Stick20ActionResetUserPassword);
    }

// Add secure password dialog test
//    trayMenu->addAction(SecPasswordAction);

    // Add debug window ?
    if (TRUE == DebugWindowActive)
    {
        trayMenu->addSeparator();
//        trayMenu->addAction(Stick20Action);           // Old command dialog
        trayMenu->addAction(Stick20ActionDebugAction);
    }


// Setup OTP combo box
    generateComboBoxEntrys ();

}


/*******************************************************************************

  generateHOTPConfig

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::generateHOTPConfig(HOTPSlot *slot)
{
    uint8_t selectedSlot = ui->slotComboBox->currentIndex();
    selectedSlot -= (TOTP_SlotCount+1);


    if (selectedSlot < HOTP_SlotCount)
    {
        slot->slotNumber=selectedSlot+0x10;


        QByteArray secretFromGUI = ui->secretEdit->text().toLatin1();

        uint8_t encoded[128];
        uint8_t decoded[20];
        uint8_t data[128];

        memset(encoded,'A',32);
        memset(data,'A',32);
        memcpy(encoded,secretFromGUI.data(),secretFromGUI.length());

        base32_clean(encoded,32,data);
        base32_decode(data,decoded,20);

        secretFromGUI = QByteArray((char *)decoded,20);//.toHex();

        memset(slot->secret,0,20);
        memcpy(slot->secret,secretFromGUI.data(),secretFromGUI.size());

        QByteArray slotNameFromGUI = QByteArray(ui->nameEdit->text().toLatin1());
        memset(slot->slotName,0,15);
        memcpy(slot->slotName,slotNameFromGUI.data(),slotNameFromGUI.size());

        memset(slot->tokenID,32,13);

        QByteArray ompFromGUI = (ui->ompEdit->text().toLatin1());
        memcpy(slot->tokenID,ompFromGUI,2);

        QByteArray ttFromGUI = (ui->ttEdit->text().toLatin1());
        memcpy(slot->tokenID+2,ttFromGUI,2);

        QByteArray muiFromGUI = (ui->muiEdit->text().toLatin1());
        memcpy(slot->tokenID+4,muiFromGUI,8);

        slot->tokenID[12] = ui->keyboardComboBox->currentIndex()&0xFF;

        QByteArray counterFromGUI = QByteArray(ui->counterEdit->text().toLatin1());
        memset(slot->counter,0,8);

        if(0 != counterFromGUI.length())
        {
            memcpy(slot->counter,counterFromGUI.data(),counterFromGUI.length());
        }
/*
        qDebug() << "Write HOTP counter " < selectedSlot;
        qDebug() << ui->counterEdit->text().toLatin1();
        qDebug() << QString ((char*)slot->counter);
        qDebug() << counterFromGUI.length();
*/
        slot->config=0;

        if (TRUE == ui->digits8radioButton->isChecked())
        {
            slot->config += (1<<0);
        }

        if (TRUE == ui->enterCheckBox->isChecked())
        {
            slot->config += (1<<1);
        }

        if (TRUE == ui->tokenIDCheckBox->isChecked())
        {
            slot->config += (1<<2);
        }

    }
   // qDebug() << slot->counter;

}

/*******************************************************************************

  generateTOTPConfig

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::generateTOTPConfig(TOTPSlot *slot)
{
    uint8_t selectedSlot = ui->slotComboBox->currentIndex();

    // get the TOTP slot number
    if (selectedSlot < TOTP_SlotCount)
    {
        slot->slotNumber = selectedSlot + 0x20;

        QByteArray secretFromGUI = ui->secretEdit->text().toLatin1();
        	
        uint8_t encoded[128];
        uint8_t decoded[20];
        uint8_t data[128];
        memset(encoded,'A',32);
        memset(data,'A',32);
        memcpy(encoded,secretFromGUI.data(),secretFromGUI.length());

        base32_clean(encoded,32,data);
        base32_decode(data,decoded,20);

        secretFromGUI = QByteArray((char *)decoded,20);//.toHex();

        memset(slot->secret,0,20);
        memcpy(slot->secret,secretFromGUI.data(),secretFromGUI.size());

        QByteArray slotNameFromGUI = QByteArray(ui->nameEdit->text().toLatin1());
        memset(slot->slotName,0,15);
        memcpy(slot->slotName,slotNameFromGUI.data(),slotNameFromGUI.size());

        memset(slot->tokenID,32,13);

        QByteArray ompFromGUI = (ui->ompEdit->text().toLatin1());
        memcpy(slot->tokenID,ompFromGUI,2);

        QByteArray ttFromGUI = (ui->ttEdit->text().toLatin1());
        memcpy(slot->tokenID+2,ttFromGUI,2);

        QByteArray muiFromGUI = (ui->muiEdit->text().toLatin1());
        memcpy(slot->tokenID+4,muiFromGUI,8);

        slot->config=0;

        if (ui->digits8radioButton->isChecked())
            slot->config+=(1<<0);
        if (ui->enterCheckBox->isChecked())
            slot->config+=(1<<1);
        if (ui->tokenIDCheckBox->isChecked())
            slot->config+=(1<<2);

        uint16_t lastInterval = ui->intervalSpinBox->value();

        if (lastInterval<1)
            lastInterval=1;

        slot->interval = lastInterval;

    }
}

/*******************************************************************************

  generateAllConfigs

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::generateAllConfigs()
{
    cryptostick->initializeConfig();
    cryptostick->getSlotConfigs();
    displayCurrentSlotConfig();
    generateMenu();
}

/*******************************************************************************

  displayCurrentTotpSlotConfig

  Changes
  Date      Author        Info
  20.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::displayCurrentTotpSlotConfig(uint8_t slotNo)
{
    //ui->hotpGroupBox->hide();
    //ui->hotpGroupBox->setTitle("Parameters");
    ui->label_5->setText("TOTP length:");
    ui->label_6->hide();
    ui->counterEdit->hide();
    ui->setToZeroButton->hide();
    ui->setToRandomButton->hide();
    ui->enterCheckBox->hide();
    ui->labelNotify->hide();
    ui->intervalLabel->show();
    ui->intervalSpinBox->show();
    ui->checkBox->setEnabled(false);
    ui->secretEdit->setPlaceholderText("********************************");

    ui->nameEdit->setText(QString((char *)cryptostick->TOTPSlots[slotNo]->slotName));
/*
    if (0 == ui->nameEdit->text().length())
    {
        ui->writeButton->setEnabled(false);
    }
    else
    {
        ui->writeButton->setEnabled(true);
    }
*/
    QByteArray secret((char *) cryptostick->TOTPSlots[slotNo]->secret,20);
    ui->base32RadioButton->setChecked(true);
    ui->secretEdit->setText(secret);//.toHex());

    ui->counterEdit->setText("0");

    QByteArray omp((char *)cryptostick->TOTPSlots[slotNo]->tokenID,2);
    ui->ompEdit->setText(QString(omp));

    QByteArray tt((char *)cryptostick->TOTPSlots[slotNo]->tokenID+2,2);
    ui->ttEdit->setText(QString(tt));

    QByteArray mui((char *)cryptostick->TOTPSlots[slotNo]->tokenID+4,8);
    ui->muiEdit->setText(QString(mui));

    int interval = cryptostick->TOTPSlots[slotNo]->interval;
    ui->intervalSpinBox->setValue(interval);

    if (cryptostick->TOTPSlots[slotNo]->config&(1<<0))
        ui->digits8radioButton->setChecked(true);
    else ui->digits6radioButton->setChecked(true);

    if (cryptostick->TOTPSlots[slotNo]->config&(1<<1))
        ui->enterCheckBox->setChecked(true);
    else ui->enterCheckBox->setChecked(false);

    if (cryptostick->TOTPSlots[slotNo]->config&(1<<2))
        ui->tokenIDCheckBox->setChecked(true);
    else ui->tokenIDCheckBox->setChecked(false);

    if (!cryptostick->TOTPSlots[slotNo]->isProgrammed){
        ui->ompEdit->setText("NK");
        ui->ttEdit->setText("01");
        QByteArray cardSerial = QByteArray((char *) cryptostick->cardSerial).toHex();
        ui->muiEdit->setText(QString( "%1" ).arg(QString(cardSerial),8,'0'));
    }
}

/*******************************************************************************

  displayCurrentHotpSlotConfig

  Changes
  Date      Author        Info
  20.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::displayCurrentHotpSlotConfig(uint8_t slotNo)
{
    //ui->hotpGroupBox->show();
    //ui->hotpGroupBox->setTitle("Parameters");
    ui->label_5->setText("HOTP length:");
    ui->label_6->show();
    ui->counterEdit->show();
    ui->setToZeroButton->show();
    ui->setToRandomButton->show();
    ui->enterCheckBox->show();
    ui->labelNotify->hide();
    ui->intervalLabel->hide();
    ui->intervalSpinBox->hide();
    ui->checkBox->setEnabled(false);
    ui->secretEdit->setPlaceholderText("********************************");
//        ui->counterEdit->setText("0");

    //slotNo=slotNo+0x10;
    ui->nameEdit->setText(QString((char *)cryptostick->HOTPSlots[slotNo]->slotName));
/*
    if (0 == ui->nameEdit->text().length())
    {
        ui->writeButton->setEnabled(false);
    }
    else
    {
        ui->writeButton->setEnabled(true);
    }
*/
    QByteArray secret((char *) cryptostick->HOTPSlots[slotNo]->secret,20);
    ui->base32RadioButton->setChecked(true);
    ui->secretEdit->setText(secret);//.toHex());

    QByteArray counter((char *) cryptostick->HOTPSlots[slotNo]->counter,8);

//    qDebug() << (char *) cryptostick->HOTPSlots[slotNo]->counter;
    QString TextCount;

    TextCount = QString ("%1").arg(counter.toInt());
//    qDebug() << TextCount;
    ui->counterEdit->setText(TextCount);//.toHex());

    QByteArray omp((char *)cryptostick->HOTPSlots[slotNo]->tokenID,2);
    ui->ompEdit->setText(QString(omp));

    QByteArray tt((char *)cryptostick->HOTPSlots[slotNo]->tokenID+2,2);
    ui->ttEdit->setText(QString(tt));

    QByteArray mui((char *)cryptostick->HOTPSlots[slotNo]->tokenID+4,8);
    ui->muiEdit->setText(QString(mui));

    if (cryptostick->HOTPSlots[slotNo]->config&(1<<0))
        ui->digits8radioButton->setChecked(true);
    else ui->digits6radioButton->setChecked(true);

    if (cryptostick->HOTPSlots[slotNo]->config&(1<<1))
        ui->enterCheckBox->setChecked(true);
    else ui->enterCheckBox->setChecked(false);

    if (cryptostick->HOTPSlots[slotNo]->config&(1<<2))
        ui->tokenIDCheckBox->setChecked(true);
    else ui->tokenIDCheckBox->setChecked(false);

    if (!cryptostick->HOTPSlots[slotNo]->isProgrammed){
        ui->ompEdit->setText("NK");
        ui->ttEdit->setText("01");
        QByteArray cardSerial = QByteArray((char *) cryptostick->cardSerial).toHex();
        ui->muiEdit->setText(QString( "%1" ).arg(QString(cardSerial),8,'0'));
    }

    //qDebug() << "Counter value:" << cryptostick->HOTPSlots[slotNo]->counter;

}

/*******************************************************************************

  displayCurrentSlotConfig

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::displayCurrentSlotConfig()
{
    uint8_t slotNo = ui->slotComboBox->currentIndex();

    if (slotNo == 255 )
    {
        return;
    }

    if(slotNo > TOTP_SlotCount)
    {
        slotNo -= (TOTP_SlotCount + 1);
    } else
    {
        slotNo += HOTP_SlotCount;
    }

    if (slotNo < HOTP_SlotCount)
    {
        displayCurrentHotpSlotConfig (slotNo);
    }
    else if ((slotNo >= HOTP_SlotCount) && (slotNo < HOTP_SlotCount + TOTP_SlotCount))
    {
        slotNo -= HOTP_SlotCount;
        displayCurrentTotpSlotConfig (slotNo);
    }

    lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
}

/*******************************************************************************

  displayCurrentGeneralConfig

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/


void MainWindow::displayCurrentGeneralConfig()
{
    ui->numLockComboBox->setCurrentIndex(0);
    ui->capsLockComboBox->setCurrentIndex(0);
    ui->scrollLockComboBox->setCurrentIndex(0);

    if (cryptostick->generalConfig[0]==0||cryptostick->generalConfig[0]==1)
        ui->numLockComboBox->setCurrentIndex(cryptostick->generalConfig[0]+1);

    if (cryptostick->generalConfig[1]==0||cryptostick->generalConfig[1]==1)
        ui->capsLockComboBox->setCurrentIndex(cryptostick->generalConfig[1]+1);

    if (cryptostick->generalConfig[2]==0||cryptostick->generalConfig[2]==1)
        ui->scrollLockComboBox->setCurrentIndex(cryptostick->generalConfig[2]+1);

    if (cryptostick->otpPasswordConfig[0] == 1)
        ui->enableUserPasswordCheckBox->setChecked(true);
    else
        ui->enableUserPasswordCheckBox->setChecked(false);

    if (cryptostick->otpPasswordConfig[1] == 1)
        ui->deleteUserPasswordCheckBox->setChecked(true);
    else
        ui->deleteUserPasswordCheckBox->setChecked(false);

    lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();

}

/*******************************************************************************

  startConfiguration

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::startConfiguration()
{

    //PasswordDialog pd;
    //pd.exec();
    bool ok;
    //int i;
/*
// Setup OTP combo box
    ui->slotComboBox->clear();

    for (i=0;i<HOTP_SlotCount;i++)
    {
        ui->slotComboBox->addItem(QString("HOTP slot ").append(QString::number(i+1,10)).append(" [").append((char *)cryptostick->HOTPSlots[i]->slotName).append("]"));
    }

    for (i=0;i<TOTP_SlotCount;i++)
    {
        ui->slotComboBox->addItem(QString("TOTP slot ").append(QString::number(i+1,10)).append(" [").append((char *)cryptostick->TOTPSlots[i]->slotName).append("]"));
    }
    ui->slotComboBox->setCurrentIndex(0);

    i = ui->slotComboBox->currentIndex();
*/

    if (!cryptostick->validPassword){
        do {
            PinDialog dialog("Enter card admin PIN", "Admin PIN:", cryptostick, PinDialog::PLAIN, PinDialog::ADMIN_PIN);
            ok = dialog.exec();
            QString password;
            dialog.getPassword(password);

            if (QDialog::Accepted == ok)
            {
                uint8_t tempPassword[25];

                for (int i=0;i<25;i++)
                    tempPassword[i]=qrand()&0xFF;

                cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(),tempPassword);
                if (cryptostick->validPassword){
                    lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
                } else {
                    csApplet->warningBox("Wrong Pin. Please try again.");
                }
                password.clear();
            }
        } while(QDialog::Accepted == ok && !cryptostick->validPassword); // While the user keeps enterning a pin and the pin is not correct..
    }

// Start the config dialog
    if (cryptostick->validPassword){
        cryptostick->getSlotConfigs();
        displayCurrentSlotConfig();

        cryptostick->getStatus();
        displayCurrentGeneralConfig();

        SetupPasswordSafeConfig ();

        showNormal();
    }
}


/*******************************************************************************

  destroyPasswordSafeStick10

  Reviews
  Date      Reviewer        Info
  03.1.14  GG              First review

*******************************************************************************/
void MainWindow::destroyPasswordSafeStick10()
{
    uint8_t password[40];
    bool    ret;
    int     ret_s32;

    QMessageBox msgBox;
    PasswordDialog dialog(FALSE,this);
    cryptostick->getUserPasswordRetryCount();
    dialog.init((char *)"Enter admin PIN", HID_Stick20Configuration_st.UserPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
        dialog.getPassword ((char*)password);

        ret_s32 = cryptostick->buildAesKey( (uint8_t*)&(password[1]) );

        if (CMD_STATUS_OK == ret_s32) 
        {
            msgBox.setText("AES key generated");
            msgBox.exec();
        }
        else
        {
            if ( CMD_STATUS_WRONG_PASSWORD == ret_s32)
                msgBox.setText("Wrong passowrd");
            else
                msgBox.setText("Unable to create new AES key");

            msgBox.exec();
        }
    }

}


/*******************************************************************************

  startStick20Configuration

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::startStick20Configuration()
{
    Stick20Dialog dialog(this);
    dialog.cryptostick=cryptostick;
    dialog.exec();
}

/*******************************************************************************

  startStickDebug

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::startStickDebug()
{
    DebugDialog dialog(this);

    dialog.cryptostick=cryptostick;

    dialog.updateText (); // Init data

    dialog.exec();
}

/*******************************************************************************

  startAboutDialog

  Changes
  Date      Author          Info
  22.07.14  RB              Move stick comunication in this context, to avoid exception

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::startAboutDialog()
{
    AboutDialog dialog(cryptostick,this);

    if (TRUE == cryptostick->activStick20)
    {
    // Get actual data from stick 20
        cryptostick->stick20GetStatusData ();

        Stick20ResponseTask ResponseTask(this,cryptostick,trayIcon);
        ResponseTask.NoStopWhenStatusOK ();
        ResponseTask.GetResponse ();

        UpdateDynamicMenuEntrys ();             // Use new data to update menu
    }

// Show dialog
    dialog.exec();
}


/*******************************************************************************

  startStick20Setup

  For testing only

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::startStick20Setup()
{
    Stick20Setup dialog(this);

    dialog.cryptostick=cryptostick;

    dialog.exec();
}

/*******************************************************************************

  startMatrixPasswordDialog

  For testing only

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::startMatrixPasswordDialog()
{
    MatrixPasswordDialog dialog(this);

    dialog.cryptostick=cryptostick;
    dialog.PasswordLen=6;
    dialog.SetupInterfaceFlag = FALSE;

    dialog.InitSecurePasswordDialog ();

    dialog.exec();
}


/*******************************************************************************

  startStick20EnableCryptedVolume

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20EnableCryptedVolume()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool ret;
    bool answer;

    if (TRUE == HiddenVolumeActive)
    {
        answer = csApplet->yesOrNoBox("This activity locks your hidden volume. Do you want to proceed?\nTo avoid data loss, please unmount the partitions before proceeding.",
                                         0, false);  
        if (false == answer)
            return;
    }

    PinDialog dialog("User pin dialog", "Enter user PIN:", cryptostick, PinDialog::PREFIXED, PinDialog::USER_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_ENABLE_CRYPTED_PARI,password);
    }
}

/*******************************************************************************

  startStick20DisableCryptedVolume

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20DisableCryptedVolume()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool answer;

    if (TRUE == CryptedVolumeActive)
    {
        answer = csApplet->yesOrNoBox("This activity locks your encrypted volume. Do you want to proceed?\nTo avoid data loss, please unmount the partitions before proceeding.",
                                        0, false);
        if (false == answer)
        {
            return;
        }
        password[0] = 0;
        stick20SendCommand (STICK20_CMD_DISABLE_CRYPTED_PARI,password);
    }
}

/*******************************************************************************

  startStick20EnableHiddenVolume

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20EnableHiddenVolume()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;
    bool answer;

    if (FALSE == CryptedVolumeActive)
    {
        csApplet->warningBox("Please enable the encrypted volume first.");
        return;
    }

    answer = csApplet->yesOrNoBox("This activity locks your encrypted volume. Do you want to proceed?\nTo avoid data loss, please unmount the partitions before proceeding.",
                                    0, true);
    if (false == answer)
        return;

    PinDialog dialog("Enter password for hidden volume", "Enter password for hidden volume:", cryptostick, PinDialog::PREFIXED, PinDialog::OTHER);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
//        password[0] = 'P';
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI,password);
    }
}

/*******************************************************************************

  startStick20DisableHiddenVolume

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20DisableHiddenVolume()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool answer;

    answer = csApplet->yesOrNoBox("This activity locks your hidden volume. Do you want to proceed?\nTo avoid data loss, please unmount the partitions before proceeding.",
                                    0, true);
    if (false == answer)
    {
        return;
    }

    password[0] = 0;
    stick20SendCommand (STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI,password);

}

/*******************************************************************************

  startLockDevice

  Changes
  Date      Author        Info
  18.10.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startLockDeviceAction()
{
    bool answer;
    if ((TRUE == CryptedVolumeActive) || (TRUE == HiddenVolumeActive))
    {
        answer = csApplet->yesOrNoBox("This activity locks your encrypted volume. Do you want to proceed?\nTo avoid data loss, please unmount the partitions before proceeding.",
                                        0, true);
        if (false == answer)
        {
            return;
        }
    }

    if ( cryptostick->lockDevice () ) {
        cryptostick->passwordSafeUnlocked=false;
    }

    HID_Stick20Configuration_st.VolumeActiceFlag_u8 = 0;

    UpdateDynamicMenuEntrys ();
}

/*******************************************************************************

  startStick20EnableFirmwareUpdate

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20EnableFirmwareUpdate()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;

    UpdateDialog dialogUpdate(this);
    ret = dialogUpdate.exec();
    if (QDialog::Accepted != ret)
    {
        return;
    }

    PinDialog dialog("Enter admin PIN", "Enter admin PIN:", cryptostick, PinDialog::PREFIXED, PinDialog::ADMIN_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_ENABLE_FIRMWARE_UPDATE,password);
    }
}

/*******************************************************************************

  startStick20ActionChangeUserPIN

  Changes
  Date      Author        Info
  24.03.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


void MainWindow::startStick20ActionChangeUserPIN()
{
    DialogChangePassword dialog(this);

    dialog.setModal (TRUE);

    dialog.cryptostick        = cryptostick;

    dialog.PasswordKind       = STICK20_PASSWORD_KIND_USER;

    dialog.InitData ();
    dialog.exec();
}


/*******************************************************************************

  startStick20ActionChangeAdminPIN

  Changes
  Date      Author        Info
  24.03.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


void MainWindow::startStick20ActionChangeAdminPIN()
{
    DialogChangePassword dialog(this);

    dialog.setModal (TRUE);

    dialog.cryptostick        = cryptostick;

    dialog.PasswordKind       = STICK20_PASSWORD_KIND_ADMIN;

    dialog.InitData ();
    dialog.exec();
}

/*******************************************************************************

  startResetUserPassword

  Changes
  Date      Author        Info
  02.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


void MainWindow::startResetUserPassword ()
{
    DialogChangePassword dialog(this);

    dialog.setModal (TRUE);

    dialog.cryptostick        = cryptostick;

    dialog.PasswordKind       = STICK20_PASSWORD_KIND_RESET_USER;

    dialog.InitData ();
    dialog.exec();
}



/*******************************************************************************

  startStick20ExportFirmwareToFile

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20ExportFirmwareToFile()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;

    PinDialog dialog("Enter admin PIN", "Enter admin PIN:", cryptostick, PinDialog::PREFIXED, PinDialog::ADMIN_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
//        password[0] = 'P';
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_EXPORT_FIRMWARE_TO_FILE, password);
    }
}

/*******************************************************************************

  startStick20DestroyCryptedVolume

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20DestroyCryptedVolume()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    int     ret;
    bool answer;

    answer = csApplet->yesOrNoBox("WARNING: Generating new AES keys will destroy the encrypted volumes, hidden volumes, and password safe! Continue?",
                                    0, false);
    if (true == answer)
    {
        PinDialog dialog("Enter admin PIN", "Admin PIN:", cryptostick, PinDialog::PREFIXED, PinDialog::ADMIN_PIN);
        ret = dialog.exec();

        if (QDialog::Accepted == ret)
        {
            dialog.getPassword ((char*)password);

            stick20SendCommand (STICK20_CMD_GENERATE_NEW_KEYS,password);
        }
    }

}

/*******************************************************************************

  startStick20FillSDCardWithRandomChars

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20FillSDCardWithRandomChars()
{
    uint8_t password[40];
    bool    ret;

    PinDialog dialog("Enter admin PIN", "Admin Pin:", cryptostick, PinDialog::PREFIXED, PinDialog::ADMIN_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
//        password[0] = 'P';
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS,password);
    }
}

/*******************************************************************************

  startStick20ClearNewSdCardFound

  Changes
  Date      Author        Info
  06.05.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20ClearNewSdCardFound()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;

    PinDialog dialog("Enter admin PIN", "Enter admin PIN:", cryptostick, PinDialog::PREFIXED, PinDialog::ADMIN_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND,password);
    }
}

/*******************************************************************************

  startStick20GetStickStatus

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20GetStickStatus()
{
/*    startAboutDialog ();
*/

// Get actual data from stick 20
    cryptostick->stick20GetStatusData ();

// Wait for response

    Stick20ResponseTask ResponseTask(this,cryptostick,trayIcon);
    ResponseTask.NoStopWhenStatusOK ();
    ResponseTask.GetResponse ();

/*
    Stick20ResponseDialog ResponseDialog(this);

    ResponseDialog.NoStopWhenStatusOK ();

    ResponseDialog.cryptostick = cryptostick;
    ResponseDialog.exec();
    ResponseDialog.ResultValue;
*/
    Stick20InfoDialog InfoDialog(this);
    InfoDialog.exec();
}

/*******************************************************************************

  startStick20SetReadonlyUncryptedVolume

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20SetReadonlyUncryptedVolume()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;

    PinDialog dialog("Enter user PIN", "User PIN:", cryptostick, PinDialog::PREFIXED, PinDialog::USER_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN,password);
    }

}

/*******************************************************************************

  startStick20SetReadWriteUncryptedVolume

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20SetReadWriteUncryptedVolume()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;

    PinDialog dialog("Enter user PIN", "User PIN:", cryptostick, PinDialog::PREFIXED, PinDialog::USER_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN,password);
    }

}

/*******************************************************************************

  startStick20LockStickHardware

  Changes
  Date      Author        Info
  11.05.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20LockStickHardware()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;

    stick20LockFirmwareDialog dialog(this);

    ret = dialog.exec();
    if (QDialog::Accepted == ret)
    {
        PinDialog dialog("Enter admin PIN", "Admin PIN:", cryptostick, PinDialog::PREFIXED, PinDialog::ADMIN_PIN);
        ret = dialog.exec();

        if (QDialog::Accepted == ret)
        {
            dialog.getPassword ((char*)password);
            stick20SendCommand (STICK20_CMD_SEND_LOCK_STICK_HARDWARE,password);
        }
    }
}

/*******************************************************************************

  startStick20SetupPasswordMatrix

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void MainWindow::startStick20SetupPasswordMatrix()
{
    MatrixPasswordDialog dialog(this);

    csApplet->warningBox("The selected lines must be greater then greatest password length");

    dialog.setModal (TRUE);

    dialog.cryptostick        = cryptostick;
    dialog.SetupInterfaceFlag = TRUE;

    dialog.InitSecurePasswordDialog ();

    dialog.exec();
}

/*******************************************************************************

  startStick20DebugAction

  Function to start a action to test functions of firmware

  Changes
  Date      Author        Info
  24.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20DebugAction()
{
    //uint8_t password[40];
    //bool    ret;
    //int64_t crc;
    int ret;
//    stick20HiddenVolumeDialog HVDialog(this);


    securitydialog dialog(this);

    ret = dialog.exec();

    csApplet->warningBox("Encrypted volume is not secure.\nSelect \"Initialize keys\"");
//                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//                msgBox.setDefaultButton(QMessageBox::Yes);


    StickNotInitated  = TRUE;
    generateMenu();

/*
    ret = HVDialog.exec();

    if (true == ret)
    {
        stick20SendCommand (STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP,(unsigned char*)&HVDialog.HV_Setup_st);
    }
*/
/*
    ret = cryptostick->getPasswordSafeSlotStatus();
    if (ERR_NO_ERROR != ret)
    {
        ret = 0;
    }
*/

//    ret = cryptostick->unlockUserPassword((uint8_t*)"123456");

/*
    ret = cryptostick->getPasswordSafeSlotName(0);
    ret = cryptostick->getPasswordSafeSlotPassword(0);
    ret = cryptostick->getPasswordSafeSlotLoginName(0);

    ret = cryptostick->setPasswordSafeSlotData_1 (0,(uint8_t*)"Name1",(uint8_t*)"PPPPP");
    ret = cryptostick->setPasswordSafeSlotData_2 (0,(uint8_t*)"LN11111");

    ret = cryptostick->getPasswordSafeSlotName(0);
    ret = cryptostick->getPasswordSafeSlotPassword(0);
    ret = cryptostick->getPasswordSafeSlotLoginName(0);

    ret = cryptostick->passwordSafeEraseSlot(0);
*/


//    stick20SendCommand (STICK20_CMD_PRODUCTION_TEST,NULL);

    if (1)
    {

//        HVDialog.cryptostick=cryptostick;

//        HVDialog.exec();
//        Result = ResponseDialog.ResultValue;

//        cryptostick->getPasswordRetryCount();
//        crc = cryptostick->getSlotName(0x10);

//        Sleep::msleep(100);
//        Response *testResponse=new Response();
//        testResponse->getResponse(cryptostick);

//        if (crc==testResponse->lastCommandCRC)
/*
        {

            QMessageBox message;
            QString str;
            QByteArray *data =new QByteArray((char*)testResponse->reportBuffer,REPORT_SIZE+1);

//            str.append(QString::number(testResponse->lastCommandCRC,16));
            str.append(QString(data->toHex()));

            message.setText(str);
            message.exec();

            str.clear();
        }
*/
    }

/*
    PasswordDialog dialog(this);
    dialog.init("Enter user password");
    ret = dialog.exec();

    if (Accepted == ret)
    {
        password[0] = 'P';
        dialog.getPassword ((char*)&password[1]);

        stick20SendCommand (STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN,password);
    }
*/


}

/*******************************************************************************

  startStick20SetupHiddenVolume

  Changes
  Date      Author        Info
  25.04.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20SetupHiddenVolume()
{
    bool    ret;
    stick20HiddenVolumeDialog HVDialog(this);

    if (FALSE == CryptedVolumeActive)
    {
        csApplet->warningBox("Please enable the encrypted volume first.");
        return;
    }

    HVDialog.SdCardHighWatermark_Read_Min  = 0;
    HVDialog.SdCardHighWatermark_Read_Max  = 100;
    HVDialog.SdCardHighWatermark_Write_Min = 0;
    HVDialog.SdCardHighWatermark_Write_Max = 100;

    ret = cryptostick->getHighwaterMarkFromSdCard (&HVDialog.SdCardHighWatermark_Write_Min,
                                                   &HVDialog.SdCardHighWatermark_Write_Max,
                                                   &HVDialog.SdCardHighWatermark_Read_Min,
                                                   &HVDialog.SdCardHighWatermark_Read_Max);
/*
    qDebug () << "High water mark : WriteMin WriteMax ReadMin ReadMax";
    qDebug () << HVDialog.SdCardHighWatermark_Write_Min;
    qDebug () << HVDialog.SdCardHighWatermark_Write_Max;
    qDebug () << HVDialog.SdCardHighWatermark_Read_Min;
    qDebug () << HVDialog.SdCardHighWatermark_Read_Max;
*/
    if (ERR_NO_ERROR != ret)
    {
        ret = 0;            // Do something ?
    }

    HVDialog.setHighWaterMarkText ();

    ret = HVDialog.exec();

    if (true == ret)
    {
        stick20SendCommand (STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP,(unsigned char*)&HVDialog.HV_Setup_st);
    }
}


/*******************************************************************************

  UpdateDynamicMenuEntrys

  Changes
  Date      Author        Info
  31.03.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int MainWindow::UpdateDynamicMenuEntrys (void)
{
    if (READ_WRITE_ACTIVE == HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8)
    {
        NormalVolumeRWActive = FALSE;
    }
    else
    {
        NormalVolumeRWActive = TRUE;
    }

    if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_CRYPTED_VOLUME_BIT_PLACE)))
    {
        CryptedVolumeActive = TRUE;
    }
    else
    {
        CryptedVolumeActive = FALSE;
    }

    if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_HIDDEN_VOLUME_BIT_PLACE)))
    {
        HiddenVolumeActive  = TRUE;
    }
    else
    {
        HiddenVolumeActive  = FALSE;
    }

    if (TRUE == HID_Stick20Configuration_st.StickKeysNotInitiated)
    {
        StickNotInitated  = TRUE;
    }
    else
    {
        StickNotInitated  = FALSE;
    }

/*
  SDFillWithRandomChars_u8
  Bit 0 = 0   SD card is *** not *** filled with random chars
  Bit 0 = 1   SD card is filled with random chars
*/

    if (0 == (HID_Stick20Configuration_st.SDFillWithRandomChars_u8 & 0x01))
    {
//        qDebug () << "UpdateDynamicMenuEntrys" << HID_Stick20Configuration_st.SDFillWithRandomChars_u8 << "SdCardNotErased = TRUE";
        SdCardNotErased  = TRUE;
    }
    else
    {
//        qDebug () << "UpdateDynamicMenuEntrys" << HID_Stick20Configuration_st.SDFillWithRandomChars_u8  << "SdCardNotErased = FALSE";
        SdCardNotErased  = FALSE;
    }

    if ((0 == HID_Stick20Configuration_st.ActiveSD_CardID_u32) || (0 == HID_Stick20Configuration_st.ActiveSmartCardID_u32))
    {
        Stick20ScSdCardOnline = FALSE;                    // SD card or smartcard are not ready

        if (0 == HID_Stick20Configuration_st.ActiveSD_CardID_u32)
        {
            Stick20ActionUpdateStickStatus->setText(tr("SD card is not ready"));
        }
        if (0 == HID_Stick20Configuration_st.ActiveSmartCardID_u32)
        {
            Stick20ActionUpdateStickStatus->setText(tr("Smartcard is not ready"));
        }
        if ((0 == HID_Stick20Configuration_st.ActiveSD_CardID_u32) && (0 == HID_Stick20Configuration_st.ActiveSmartCardID_u32))
        {
            Stick20ActionUpdateStickStatus->setText(tr("Smartcard and SD card are not ready"));
        }

    }
    else
    {
        Stick20ScSdCardOnline = TRUE;
    }




    generateMenu();

    return (TRUE);
}

/*******************************************************************************

  stick20SendCommand

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

int MainWindow::stick20SendCommand (uint8_t stick20Command, uint8_t *password)
{
    int         ret;
    bool        waitForAnswerFromStick20;
    bool        stopWhenStatusOKFromStick20;
    int         Result;

    QByteArray  passwordString;

    waitForAnswerFromStick20    = FALSE;
    stopWhenStatusOKFromStick20 = FALSE;

    switch (stick20Command)
    {
        case STICK20_CMD_ENABLE_CRYPTED_PARI            :
            ret = cryptostick->stick20EnableCryptedPartition (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_DISABLE_CRYPTED_PARI           :
            ret = cryptostick->stick20DisableCryptedPartition ();
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI     :
            ret = cryptostick->stick20EnableHiddenCryptedPartition (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI    :
            ret = cryptostick->stick20DisableHiddenCryptedPartition ();
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_ENABLE_FIRMWARE_UPDATE         :
            ret = cryptostick->stick20EnableFirmwareUpdate (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = FALSE;
            }
            break;
        case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE        :
            ret = cryptostick->stick20ExportFirmware (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_GENERATE_NEW_KEYS              :
/*
            {
                bool answer = csApplet->yesOrNoBox("WARNING: Generating new AES keys will destroy the encrypted volumes, hidden volumes, and password safe! Continue?", 0, false);

                if (answer)
                {
                    ret = cryptostick->stick20CreateNewKeys (password);
                    if (TRUE == ret)
                    {
                        waitForAnswerFromStick20 = TRUE;
                    }
                }
            }
*/
            ret = cryptostick->stick20CreateNewKeys (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS :
            {
                bool answer = csApplet->yesOrNoBox(
                    "This command fills the encrypted volumes with random data.\nThis will destroy all encrypted volumes!\nThis commands requires more than 1 hour for 32GB.",
                    0, false);

                if (answer)
                {
                    ret = cryptostick->stick20FillSDCardWithRandomChars (password,STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ENCRYPTED_VOL);
                    if (TRUE == ret)
                    {
                        waitForAnswerFromStick20    = TRUE;
                        stopWhenStatusOKFromStick20 = FALSE;
                    }
                }
            }
            break;
         case STICK20_CMD_WRITE_STATUS_DATA :
            csApplet->messageBox("Not implemented");
            break;
        case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN :
            ret = cryptostick->stick20SendSetReadonlyToUncryptedVolume (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
            ret = cryptostick->stick20SendSetReadwriteToUncryptedVolume (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_SEND_PASSWORD_MATRIX        :
            ret = cryptostick->stick20GetPasswordMatrix ();
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA        :
            ret = cryptostick->stick20SendPasswordMatrixPinData (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_GET_DEVICE_STATUS        :
            ret = cryptostick->stick20GetStatusData ();
            if (TRUE == ret)
            {
                waitForAnswerFromStick20    = TRUE;
                stopWhenStatusOKFromStick20 = FALSE;
            }
            break;
        case STICK20_CMD_SEND_STARTUP                   :
            break;

        case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP :
            ret = cryptostick->stick20SendHiddenVolumeSetup ((HiddenVolumeSetup_tst *)password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20    = TRUE;
                stopWhenStatusOKFromStick20 = FALSE;
            }
            break;

        case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND        :
            ret = cryptostick->stick20SendClearNewSdCardFound(password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_SEND_LOCK_STICK_HARDWARE       :
            ret = cryptostick->stick20LockFirmware (password);
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;
        case STICK20_CMD_PRODUCTION_TEST       :
            ret = cryptostick->stick20ProductionTest();
            if (TRUE == ret)
            {
                waitForAnswerFromStick20 = TRUE;
            }
            break;

        default :
            csApplet->messageBox("Stick20Dialog: Wrong combobox value! ");
            break;

    }

    Result = FALSE;
    if (TRUE == waitForAnswerFromStick20)
    {

        Stick20ResponseTask ResponseTask(this,cryptostick,trayIcon);
        if (FALSE == stopWhenStatusOKFromStick20)
        {
            ResponseTask.NoStopWhenStatusOK ();
        }
        ResponseTask.GetResponse ();
        Result = ResponseTask.ResultValue;
//qDebug()<< "waitForAnswerFromStick20" << ResponseTask.ResultValue;

    }

    if (TRUE == Result)
    {
        switch (stick20Command)
        {
            case STICK20_CMD_ENABLE_CRYPTED_PARI            :
                HID_Stick20Configuration_st.VolumeActiceFlag_u8 = (1 << SD_CRYPTED_VOLUME_BIT_PLACE);
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_DISABLE_CRYPTED_PARI           :
                HID_Stick20Configuration_st.VolumeActiceFlag_u8 = 0;
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI     :
                HID_Stick20Configuration_st.VolumeActiceFlag_u8 = (1 << SD_HIDDEN_VOLUME_BIT_PLACE);
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI    :
                HID_Stick20Configuration_st.VolumeActiceFlag_u8 = 0;
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_GET_DEVICE_STATUS              :
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_SEND_STARTUP                   :
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
               HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8 = READ_WRITE_ACTIVE;
               UpdateDynamicMenuEntrys ();
               break;
            case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
               HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8 = READ_ONLY_ACTIVE;
               UpdateDynamicMenuEntrys ();
               break;
            case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND        :
               HID_Stick20Configuration_st.SDFillWithRandomChars_u8 |= 0x01;
               UpdateDynamicMenuEntrys ();
               break;
            case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS :
                HID_Stick20Configuration_st.SDFillWithRandomChars_u8 |= 0x01;
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_GENERATE_NEW_KEYS              : // = firmware reset
                HID_Stick20Configuration_st.StickKeysNotInitiated    = FALSE;
                HID_Stick20Configuration_st.SDFillWithRandomChars_u8 = 0x00;
                HID_Stick20Configuration_st.VolumeActiceFlag_u8      = 0;
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_PRODUCTION_TEST       :
                break;

            default :
                break;
        }
    }


    return (true);
}




/*******************************************************************************

  getCode

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::getCode(uint8_t slotNo)
{
    uint8_t result[18];
    memset(result,0,18);
    uint32_t code;


     cryptostick->getCode(slotNo,currentTime/30,currentTime,30,result);
     //cryptostick->getCode(slotNo,1,result);
     code=result[0]+(result[1]<<8)+(result[2]<<16)+(result[3]<<24);
     code=code%100000000;

//     qDebug() << "Current time:" << currentTime;
//     qDebug() << "Counter:" << currentTime/30;
//     qDebug() << "TOTP:" << code;

}

/*******************************************************************************

  on_writeButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_writeButton_clicked()
{
    int res;
    uint8_t SlotName[16];

    uint8_t slotNo = ui->slotComboBox->currentIndex();

    if(slotNo > TOTP_SlotCount)
    {
        slotNo -= (TOTP_SlotCount + 1);
    } else
    {
        slotNo += HOTP_SlotCount;
    }

    STRNCPY ((char*)SlotName,sizeof (SlotName),ui->nameEdit->text().toLatin1(),15);

    SlotName[15] = 0;
    if (0 == strlen ((char*)SlotName))
    {
        csApplet->warningBox("Please enter a slotname.");
        return;
    }

    if (true == cryptostick->isConnected)
    {
        ui->base32RadioButton->toggle();

        if (slotNo < HOTP_SlotCount){//HOTP slot
            HOTPSlot *hotp=new HOTPSlot();

            generateHOTPConfig(hotp);
            //HOTPSlot *hotp=new HOTPSlot(0x10,(uint8_t *)"Herp",(uint8_t *)"123456",(uint8_t *)"0",0);
            res = cryptostick->writeToHOTPSlot(hotp);

        }
        else{//TOTP slot
            TOTPSlot *totp=new TOTPSlot();
            generateTOTPConfig(totp);
            res = cryptostick->writeToTOTPSlot(totp);
        }

        if (res==0)
            csApplet->messageBox("Configuration has been written successfully.");
        else if (res == -3)
            csApplet->warningBox("The name of the slot must not be empty.");
        else
            csApplet->warningBox("Error writing config!");


        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        Sleep::msleep(500);
        QApplication::restoreOverrideCursor();

        generateAllConfigs();

    }
    else
    {
        csApplet->warningBox("Crypto Stick is not connected!");
    }

    displayCurrentSlotConfig();
}

/*******************************************************************************

  on_slotComboBox_currentIndexChanged

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_slotComboBox_currentIndexChanged(int index)
{
    index = index;      // avoid warning

    displayCurrentSlotConfig();
}

/*******************************************************************************

  on_resetButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

/*void MainWindow::on_resetButton_clicked()
{
    displayCurrentSlotConfig();
}*/

/*******************************************************************************

  on_hexRadioButton_toggled

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/


void MainWindow::on_hexRadioButton_toggled(bool checked)
{
    QByteArray secret;
    uint8_t encoded[128];
    uint8_t data[128];
    uint8_t decoded[20];

    if (checked){

        secret = ui->secretEdit->text().toLatin1();
// qDebug() << "encoded secret:" << QString(secret);
        if(secret.size() != 0){
        memset(encoded,'A',32);
        memcpy(encoded,secret.data(),secret.length());
        
        base32_clean(encoded,32,data);
        base32_decode(data,decoded,20);

        //ui->secretEdit->setInputMask("HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH HH;");
        //ui->secretEdit->setInputMask("hh hh hh hh hh hh hh hh hh hh hh hh hh hh hh hh hh hh hh hh;");
        //ui->secretEdit->setInputMask("HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH;0");
        //ui->secretEdit->setMaxLength(59);

        secret = QByteArray((char *)decoded,20).toHex();

        //ui->secretEdit->setMaxLength(20);
        ui->secretEdit->setText(QString(secret));
        secretInClipboard = ui->secretEdit->text();
        copyToClipboard(secretInClipboard);
//qDebug() << QString(secret);
        }
    }
}

/*******************************************************************************

  on_base32RadioButton_toggled

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_base32RadioButton_toggled(bool checked)
{
    QByteArray secret;
    uint8_t encoded[128];
    uint8_t decoded[20];
    if (checked){ 
        secret = QByteArray::fromHex(ui->secretEdit->text().toLatin1());
        if(secret.size() != 0){
        memset(decoded,0,20);
        memcpy(decoded,secret.data(),secret.length());

        base32_encode(decoded,secret.length(),encoded,128);

        //ui->secretEdit->setInputMask("NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN;");
        //ui->secretEdit->setInputMask("nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn;");
        //ui->secretEdit->setMaxLength(32);
        ui->secretEdit->setText(QString((char *)encoded));
        secretInClipboard = ui->secretEdit->text();
        copyToClipboard(secretInClipboard);
//qDebug() << QString((char *)encoded);
        }
    }
}

/*******************************************************************************

  on_setToZeroButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/


void MainWindow::on_setToZeroButton_clicked()
{
    ui->counterEdit->setText("0");
}

/*******************************************************************************

  on_setToRandomButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_setToRandomButton_clicked()
{
    quint64 counter;

    counter = qrand() & 0xFFFF;
    counter *= 16;

    //qDebug() << counter;

    ui->counterEdit->setText(QString(QByteArray::number(counter,10)));
}

/*******************************************************************************

  on_checkBox_2_toggled

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/
/*
void MainWindow::on_checkBox_2_toggled(bool checked)
{
    checked = checked;      // avoid warning
}
*/
/*******************************************************************************

  on_tokenIDCheckBox_toggled

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_tokenIDCheckBox_toggled(bool checked)
{

    if (checked){
        ui->ompEdit->setEnabled(true);
        ui->ttEdit->setEnabled(true);
        ui->muiEdit->setEnabled(true);


    }
    else{
        ui->ompEdit->setEnabled(false);
        ui->ttEdit->setEnabled(false);
        ui->muiEdit->setEnabled(false);


    }
}

/*******************************************************************************

  on_enableUserPasswordCheckBox_toggled

  Reviews
  Date      Reviewer        Info
  12.08.14  SN              First review

*******************************************************************************/

void MainWindow::on_enableUserPasswordCheckBox_toggled(bool checked)
{

    if (checked){
        ui->deleteUserPasswordCheckBox->setEnabled(true);
        ui->deleteUserPasswordCheckBox->setChecked(false);
        cryptostick->otpPasswordConfig[0] = 1;
        cryptostick->otpPasswordConfig[1] = 0;

    }
    else{
        ui->deleteUserPasswordCheckBox->setEnabled(false);
        ui->deleteUserPasswordCheckBox->setChecked(false);
        cryptostick->otpPasswordConfig[0] = 0;
        cryptostick->otpPasswordConfig[1] = 0;
    }

}


/*******************************************************************************

  on_writeGeneralConfigButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_writeGeneralConfigButton_clicked()
{
    int res;
    uint8_t data[5];

    if (cryptostick->isConnected){

        data[0]=ui->numLockComboBox->currentIndex()-1;
        data[1]=ui->capsLockComboBox->currentIndex()-1;
        data[2]=ui->scrollLockComboBox->currentIndex()-1;

        if(ui->enableUserPasswordCheckBox->isChecked())
        {
            data[3]=1;
            if(ui->deleteUserPasswordCheckBox->isChecked())
            {
                data[4]=1;
            }
            else
            {
                data[4]=0;
            }
        } else
        {
            data[3]=0;
            data[4]=0;
        }

        res =cryptostick->writeGeneralConfig(data);

        if (res==0)
            csApplet->messageBox("Config written successfully.");
        else
            csApplet->warningBox("Error writing config!");

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        Sleep::msleep(500);
        QApplication::restoreOverrideCursor();
        cryptostick->getStatus();
        generateAllConfigs();

    }
    else{
        csApplet->warningBox("Crypto Stick not connected!");
    }
    displayCurrentGeneralConfig();
}
/*******************************************************************************

  getHOTPDialog

  Changes
  Date      Author          Info
  25.03.14  RB              Dynamic slot counts

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::getHOTPDialog(int slot)
{
    int ret;

    ret = getNextCode(0x10 + slot);

    if(ret == 0)
    {
        if(cryptostick->HOTPSlots[slot]->slotName[0] == '\0')
            trayIcon->showMessage (QString("HOTP slot ").append(QString::number(slot+1,10)),"One-time password has been copied to clipboard.",QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
        else
            trayIcon->showMessage (QString("HOTP slot ").append(QString::number(slot+1,10)).append(" [").append((char *)cryptostick->HOTPSlots[slot]->slotName).append("]"),
                                    "One-time password has been copied to clipboard.",QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
    }
}

void MainWindow::getHOTP1()
{
    getHOTPDialog (0);
}
void MainWindow::getHOTP2()
{
    getHOTPDialog (1);
}
void MainWindow::getHOTP3()
{
    getHOTPDialog (2);
}

/*******************************************************************************

  getHOTP1

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/
/*
void MainWindow::getHOTP1()
{
    HOTPDialog dialog(this);
    dialog.device=cryptostick;
    dialog.slotNumber=0x10;
    dialog.title=QString("HOTP slot 1 [").append((char *)cryptostick->HOTPSlots[0]->slotName).append("]");
    dialog.setToHOTP();
    dialog.getNextCode();
    dialog.exec();
}
*/
/*******************************************************************************

  getHOTP2

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/
/*
void MainWindow::getHOTP2()
{
    HOTPDialog dialog(this);
    dialog.device=cryptostick;
    dialog.slotNumber=0x11;
    dialog.title=QString("HOTP slot 2 [").append((char *)cryptostick->HOTPSlots[1]->slotName).append("]");
    dialog.setToHOTP();
    dialog.getNextCode();
    dialog.exec();
}
*/

/*******************************************************************************

  getTOTPDialog

  Changes
  Date      Author          Info
  25.03.14  RB              Dynamic slot counts

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::getTOTPDialog(int slot)
{
    //HOTPDialog dialog(this);
    //dialog.device=cryptostick;
    //dialog.slotNumber=0x20 + slot;
    //dialog.title=QString("TOTP slot ").append(QString::number(slot+1,10)).append(" [").append((char *)cryptostick->TOTPSlots[slot]->slotName).append("]");
   // dialog.setToTOTP();
    //dialog.getNextCode();
   //dialog.exec();

    int ret;

    ret = getNextCode(0x20 + slot);
    if(ret == 0){
    if(cryptostick->TOTPSlots[slot]->slotName[0] == '\0')
        trayIcon->showMessage (QString("TOTP slot ").append(QString::number(slot+1,10)),"One-time password has been copied to clipboard.", QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
    else
        trayIcon->showMessage (QString("TOTP slot ").append(QString::number(slot+1,10)).append(" [").append((char *)cryptostick->TOTPSlots[slot]->slotName).append("]"),
                                "One-time password has been copied to clipboard.", QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
    }
}


/*******************************************************************************

  getTOTP1

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::getTOTP1()
{
    getTOTPDialog (0);
}
void MainWindow::getTOTP2()
{
    getTOTPDialog (1);
}
void MainWindow::getTOTP3()
{
    getTOTPDialog (2);
}
void MainWindow::getTOTP4()
{
    getTOTPDialog (3);
}
void MainWindow::getTOTP5()
{
    getTOTPDialog (4);
}
void MainWindow::getTOTP6()
{
    getTOTPDialog (5);
}
void MainWindow::getTOTP7()
{
    getTOTPDialog (6);
}
void MainWindow::getTOTP8()
{
    getTOTPDialog (7);
}
void MainWindow::getTOTP9()
{
    getTOTPDialog (8);
}
void MainWindow::getTOTP10()
{
    getTOTPDialog (9);
}
void MainWindow::getTOTP11()
{
    getTOTPDialog (10);
}
void MainWindow::getTOTP12()
{
    getTOTPDialog (11);
}
void MainWindow::getTOTP13()
{
    getTOTPDialog (12);
}
void MainWindow::getTOTP14()
{
    getTOTPDialog (13);
}
void MainWindow::getTOTP15()
{
    getTOTPDialog (14);
}


/*******************************************************************************

  on_eraseButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_eraseButton_clicked()
{
     bool answer = csApplet->yesOrNoBox("WARNING: Are you sure you want to erase the slot?");
     char clean[8];

     memset(clean,' ',8);

     uint8_t slotNo = ui->slotComboBox->currentIndex();
     if(slotNo > TOTP_SlotCount){
         slotNo -= (TOTP_SlotCount + 1);
     } else {
         slotNo += HOTP_SlotCount;
     }

     if (slotNo < HOTP_SlotCount)
     {
         memcpy(cryptostick->HOTPSlots[slotNo&0x0F]->counter,clean,8);
         slotNo = slotNo+0x10;
     }
     else if ((slotNo >= HOTP_SlotCount) && (slotNo <= TOTP_SlotCount + HOTP_SlotCount))
     {
         slotNo = slotNo + 0x20 - HOTP_SlotCount;
     }

     if (answer) {
        cryptostick->eraseSlot(slotNo);
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        Sleep::msleep(1000);
        QApplication::restoreOverrideCursor();
        generateAllConfigs();

        csApplet->messageBox("Slot has been erased successfully.");
     }

     displayCurrentSlotConfig();
}

/*******************************************************************************

  on_resetGeneralConfigButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review
  13.08.13  SN              Removed

*******************************************************************************/

/*
 void MainWindow::on_resetGeneralConfigButton_clicked()
{
    displayCurrentGeneralConfig();
}
*/

/*******************************************************************************

  on_randomSecretButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_randomSecretButton_clicked()
{
    int i=0;
    uint8_t secret[32];
    char temp;


   // for (i=0;i<20;i++)
   //     secret[i]=qrand()&0xFF;
    while(i<32){
        temp = qrand()&0xFF;
        if((temp >= 'A' && temp <= 'Z') || (temp >= '2' && temp <= '7')){
            secret[i]=temp;
            i++;
        }
    }

    QByteArray secretArray((char *) secret,32);
    ui->base32RadioButton->setChecked(true);
    //ui->secretEdit->setText(secretArray.toHex());
    ui->secretEdit->setText(secretArray);
    ui->checkBox->setEnabled(true);
    ui->checkBox->setChecked(true);
    secretInClipboard = ui->secretEdit->text();
    copyToClipboard(secretInClipboard);

}

/*******************************************************************************

  on_checkBox_toggled

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_checkBox_toggled(bool checked)
{
    if (checked)
        ui->secretEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    else
        ui->secretEdit->setEchoMode(QLineEdit::Normal);


}

void MainWindow::copyToClipboard(QString text)
{
     if (text != 0){
        lastClipboardTime = QDateTime::currentDateTime().toTime_t();
        clipboard->setText(text);
        ui->labelNotify->show();
     }
    // this->accept();
}

void MainWindow::checkClipboard_Valid()
{
    uint64_t currentTime;
    //uint64_t checkTime;

    currentTime = QDateTime::currentDateTime().toTime_t();
    if((currentTime >= (lastClipboardTime + (uint64_t)60)) && (clipboard->text() == otpInClipboard)){
        otpInClipboard = "";
        clipboard->setText(QString(""));
    }

    if((currentTime >= (lastClipboardTime + (uint64_t)120)) && (clipboard->text() == secretInClipboard)){
        secretInClipboard = "";
        clipboard->setText(QString(""));
        ui->labelNotify->hide();
    }

    if (QString::compare(clipboard->text(),secretInClipboard)!=0){
        ui->labelNotify->hide();
    }
}

void MainWindow::checkPasswordTime_Valid(){
    uint64_t currentTime;


    currentTime = QDateTime::currentDateTime().toTime_t();
    if(currentTime >= lastAuthenticateTime + (uint64_t)600){
        cryptostick->validPassword = false;
        memset(cryptostick->password,0,25);
    }
    if(currentTime >= lastUserAuthenticateTime + (uint64_t)600 && cryptostick->otpPasswordConfig[0] == 1 && cryptostick->otpPasswordConfig[1] == 1){
        cryptostick->validUserPassword = false;
        memset(cryptostick->userPassword,0,25);
    }

}

void MainWindow::checkTextEdited(){
    if(!ui->checkBox->isEnabled()){
        ui->checkBox->setEnabled(true);
        ui->checkBox->setChecked(false);
    }
}

/*******************************************************************************

  SetupPasswordSafeConfig

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::SetupPasswordSafeConfig (void)
{
    int ret;
    int i;
    QString Slotname;

    ui->PWS_ComboBoxSelectSlot->clear();
    PWS_Access = FALSE;

// Get active password slots
    ret = cryptostick->getPasswordSafeSlotStatus();
    if (ERR_NO_ERROR == ret)
    {
        PWS_Access = TRUE;
    // Setup combobox
        for (i=0;i<PWS_SLOT_COUNT;i++)
        {
            if (TRUE == cryptostick->passwordSafeStatus[i])
            {
                if (0 == strlen ((char*)cryptostick->passwordSafeSlotNames[i]))
                {
                    cryptostick->getPasswordSafeSlotName(i);

                    STRCPY ((char*)cryptostick->passwordSafeSlotNames[i],sizeof (cryptostick->passwordSafeSlotNames[i]),(char*)cryptostick->passwordSafeSlotName);
                }
//                ui->PWS_ComboBoxSelectSlot->addItem((char*)cryptostick->passwordSafeSlotNames[i]);
                ui->PWS_ComboBoxSelectSlot->addItem(QString("Slot ").append(QString::number(i+1,10)).append(QString(" [").append((char*)cryptostick->passwordSafeSlotNames[i]).append(QString("]"))));
            }
            else
            {
                cryptostick->passwordSafeSlotNames[i][0] = 0;
                ui->PWS_ComboBoxSelectSlot->addItem(QString("Slot ").append(QString::number(i+1,10)));
            }
        }
    }
    else
    {
        ui->PWS_ComboBoxSelectSlot->addItem(QString("Unlock password safe"));
    }

    if (TRUE == PWS_Access)
    {
        ui->PWS_ButtonEnable->hide();
        ui->PWS_ButtonSaveSlot->setEnabled(TRUE);
        ui->PWS_ButtonClearSlot->setEnabled(TRUE);

        ui->PWS_ComboBoxSelectSlot->setEnabled(TRUE);
        ui->PWS_EditSlotName->setEnabled(TRUE);
        ui->PWS_EditLoginName->setEnabled(TRUE);
        ui->PWS_EditPassword->setEnabled(TRUE);
        ui->PWS_CheckBoxHideSecret->setEnabled(TRUE);
        ui->PWS_ButtonCreatePW->setEnabled(TRUE);
    }
    else
    {
        ui->PWS_ButtonEnable->show();
        ui->PWS_ButtonSaveSlot->setDisabled(TRUE);
        ui->PWS_ButtonClearSlot->setDisabled(TRUE);

        ui->PWS_ComboBoxSelectSlot->setDisabled(TRUE);
        ui->PWS_EditSlotName->setDisabled(TRUE);
        ui->PWS_EditLoginName->setDisabled(TRUE);
        ui->PWS_EditPassword->setDisabled(TRUE);
        ui->PWS_CheckBoxHideSecret->setDisabled(TRUE);
        ui->PWS_ButtonCreatePW->setDisabled(TRUE);

    }

    ui->PWS_EditSlotName->setMaxLength(PWS_SLOTNAME_LENGTH);
    ui->PWS_EditPassword->setMaxLength(PWS_PASSWORD_LENGTH);
    ui->PWS_EditLoginName->setMaxLength(PWS_LOGINNAME_LENGTH);

    ui->PWS_CheckBoxHideSecret->setChecked(TRUE);
    ui->PWS_EditPassword->setEchoMode(QLineEdit::Password);
}

/*******************************************************************************

  on_PWS_ButtonClearSlot_clicked

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_PWS_ButtonClearSlot_clicked()
{
    int Slot;
    unsigned int ret;
    QMessageBox msgBox;

    Slot = ui->PWS_ComboBoxSelectSlot->currentIndex();

    if (0 != cryptostick->passwordSafeSlotNames[Slot][0])      // Is slot active ?
    {
        ret = cryptostick->passwordSafeEraseSlot(Slot);


        if (ERR_NO_ERROR == ret)
        {
            ui->PWS_EditSlotName->setText("");
            ui->PWS_EditPassword->setText("");
            ui->PWS_EditLoginName->setText("");
            cryptostick->passwordSafeStatus[Slot] = FALSE;
            cryptostick->passwordSafeSlotNames[Slot][0] = 0;
            ui->PWS_ComboBoxSelectSlot->setItemText(Slot,QString("Slot ").append(QString::number(Slot+1,10)));
        }
        else
        {
            csApplet->warningBox("Can't clear slot.");
        }
    }
    else
    {
        csApplet->messageBox("Slot is erased already.");
    }

    generateMenu();
}

/*******************************************************************************

  on_PWS_ComboBoxSelectSlot_currentIndexChanged

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_PWS_ComboBoxSelectSlot_currentIndexChanged(int index)
{
    int ret;
    QString OutputText;

    if (FALSE == PWS_Access)
    {
        return;
    }

// Slot already used ?
    if (TRUE == cryptostick->passwordSafeStatus[index])
    {
//        ret = cryptostick->getPasswordSafeSlotName(index);
        ui->PWS_EditSlotName->setText((char*)cryptostick->passwordSafeSlotNames[index]);

        ret = cryptostick->getPasswordSafeSlotPassword(index);
        ret = cryptostick->getPasswordSafeSlotLoginName(index);

        ui->PWS_EditPassword->setText((QString)(char*)cryptostick->passwordSafePassword);
        ui->PWS_EditLoginName->setText((QString)(char*)cryptostick->passwordSafeLoginName);
    }
    else
    {
        ui->PWS_EditSlotName->setText("");
        ui->PWS_EditPassword->setText("");
        ui->PWS_EditLoginName->setText("");
    }
}

/*******************************************************************************

  on_PWS_CheckBoxHideSecret_toggled

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_PWS_CheckBoxHideSecret_toggled(bool checked)
{
    if (checked)
        ui->PWS_EditPassword->setEchoMode(QLineEdit::Password);
    else
        ui->PWS_EditPassword->setEchoMode(QLineEdit::Normal);
}

/*******************************************************************************

  on_PWS_ButtonSaveSlot_clicked

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_PWS_ButtonSaveSlot_clicked()
{
    int Slot;
    int ret;
    uint8_t SlotName[PWS_SLOTNAME_LENGTH+1];
    uint8_t LoginName[PWS_LOGINNAME_LENGTH+1];
    uint8_t Password[PWS_PASSWORD_LENGTH+1];
    QMessageBox msgBox;

    Slot = ui->PWS_ComboBoxSelectSlot->currentIndex();

    
    STRNCPY ((char*)SlotName,sizeof (SlotName),ui->PWS_EditSlotName->text().toLatin1(),PWS_SLOTNAME_LENGTH);
    SlotName[PWS_SLOTNAME_LENGTH] = 0;
    if (0 == strlen ((char*)SlotName))
    {
        csApplet->warningBox("Please enter a slotname.");
        return;
    }

    STRNCPY ((char*)LoginName,sizeof (LoginName),ui->PWS_EditLoginName->text().toLatin1(),PWS_LOGINNAME_LENGTH);
    LoginName[PWS_LOGINNAME_LENGTH] = 0;

    STRNCPY ((char*)Password,sizeof (Password),ui->PWS_EditPassword->text().toLatin1(),PWS_PASSWORD_LENGTH);
    Password[PWS_PASSWORD_LENGTH] = 0;
    if (0 == strlen ((char*)Password))
    {
        csApplet->warningBox("Please enter a password.");
        return;
    }

    ret = cryptostick->setPasswordSafeSlotData_1 (Slot,(uint8_t*)SlotName,(uint8_t*)Password);
    if (ERR_NO_ERROR != ret)
    {
        msgBox.setText(tr("Can't save slot. %1").arg(ret));
        msgBox.exec();
        return;
    }

    ret = cryptostick->setPasswordSafeSlotData_2 (Slot,(uint8_t*)LoginName);
    if (ERR_NO_ERROR != ret)
    {
        csApplet->warningBox("Can't save slot.");
        return;
    }

    cryptostick->passwordSafeStatus[Slot] = TRUE;
    STRCPY ((char*)cryptostick->passwordSafeSlotNames[Slot],sizeof (cryptostick->passwordSafeSlotNames[Slot]),(char*)SlotName);
    ui->PWS_ComboBoxSelectSlot->setItemText (Slot,QString("Slot ").append(QString::number(Slot+1,10)).append(QString(" [").append((char*)cryptostick->passwordSafeSlotNames[Slot]).append(QString("]"))));


    generateMenu ();
}

/*******************************************************************************

  on_PWS_ButtonClose_pressed

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

char *MainWindow::PWS_GetSlotName (int Slot)
{
    if (0 == strlen ((char*)cryptostick->passwordSafeSlotNames[Slot]))
    {
        cryptostick->getPasswordSafeSlotName(Slot);

        STRCPY ((char*)cryptostick->passwordSafeSlotNames[Slot],sizeof (cryptostick->passwordSafeSlotNames[Slot]),(char*)cryptostick->passwordSafeSlotName);
    }
    return ((char*)cryptostick->passwordSafeSlotNames[Slot]);
}

/*******************************************************************************

  on_PWS_ButtonClose_pressed

  Changes
  Date      Author        Info
  01.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_PWS_ButtonClose_pressed()
{
    hide();
}


/*******************************************************************************

  generateMenuPasswordSafe

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::generateMenuPasswordSafe()
{
    if (FALSE == cryptostick->passwordSafeUnlocked)
    {
        trayMenu->addAction(UnlockPasswordSafe);

        if(true == cryptostick->passwordSafeAvailable) {
            UnlockPasswordSafe->setEnabled(true);
        }
        else {
            UnlockPasswordSafe->setEnabled(false);
        }
        return;
    }
}

/*******************************************************************************

  PWS_Clicked_EnablePWSAccess

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::PWS_Clicked_EnablePWSAccess ()
{
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;
    int     ret_s32;


    PasswordDialog dialog(FALSE,this);
    cryptostick->getUserPasswordRetryCount();
    dialog.init((char *)"Enter user PIN",HID_Stick20Configuration_st.UserPwRetryCount);
    dialog.cryptostick = cryptostick;

//    PinDialog dialog("Enter user PIN", "User Pin:", cryptostick, PinDialog::PREFIXED, PinDialog::USER_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret)
    {
        dialog.getPassword ((char*)password);

        ret_s32 = cryptostick->isAesSupported( (uint8_t*)&password[1] );
        // ret_s32 = cryptostick->isAesSupported( password );

        if (CMD_STATUS_OK == ret_s32)   // AES supported, continue
        {
            cryptostick->passwordSafeAvailable = TRUE;
            UnlockPasswordSafe->setEnabled(true);

            // Continue to unlocking password safe
            ret_s32 = cryptostick->passwordSafeEnable ((char*)&password[1]);
            // ret_s32 = cryptostick->passwordSafeEnable ((char*)password);

            if (ERR_NO_ERROR != ret_s32)
            {
                csApplet->warningBox("Can't unlock password safe.");
            }
            else
            {
                if (TRUE == trayIcon->supportsMessages ())
                {
                    trayIcon->showMessage ("Crypto Stick Utility","Password Safe unlocked successfully.");
                }
                else
                {
                    csApplet->messageBox("Password safe is enabled");
                }

                SetupPasswordSafeConfig ();
                generateMenu ();
                ui->tabWidget->setTabEnabled(3, 1);
            }
        }
        else
        {
            if (CMD_STATUS_NOT_SUPPORTED == ret_s32 ) // AES NOT supported
            {
                // Mark password safe as disabled feature
                cryptostick->passwordSafeAvailable = FALSE;
                UnlockPasswordSafe->setEnabled(false);
                csApplet->warningBox("Password safe is not supported by this device.");
                generateMenu ();
                ui->tabWidget->setTabEnabled(3, 0);
            }
            else
            {
                if (CMD_STATUS_WRONG_PASSWORD == ret_s32) // Wrong password
                {
                    csApplet->warningBox("Wrong user password.");
                }
            }
            return;
        }
    }
}

/*******************************************************************************

  PWS_ExceClickedSlot

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::PWS_ExceClickedSlot (int Slot)
{
    QString MsgText;
    QString MsgText_1;
    int     ret_s32;

    ret_s32 = cryptostick->getPasswordSafeSlotPassword(Slot);
    if (ERR_NO_ERROR != ret_s32)
    {
        csApplet->warningBox("Pasword safe: Can't get password");
        return;
    }
    MsgText.append((char*)cryptostick->passwordSafePassword);

    clipboard->setText(MsgText);

    memset (cryptostick->passwordSafePassword,0,sizeof (cryptostick->passwordSafePassword));


    if (TRUE == trayIcon->supportsMessages ())
    {
        MsgText.sprintf("Password safe [%s]",(char*)cryptostick->passwordSafeSlotNames[Slot]);
        MsgText_1.sprintf("Password has been copied to clipboard");

        trayIcon->showMessage (MsgText,MsgText_1,QSystemTrayIcon::Information, TRAY_MSG_TIMEOUT);
    }
    else
    {
        MsgText.sprintf("Password safe [%s] has been copied to clipboard",(char*)cryptostick->passwordSafeSlotNames[Slot]);
        csApplet->messageBox(MsgText);
    }



/*
    PasswordSafeDialog PWS_dialog (Slot,this);

    PWS_dialog.cryptostick = cryptostick;

    PWS_dialog.exec();
*/
/*
    QString MsgText ("PW Safe Slot ");
    QMessageBox msgBox;
    int     ret_s32;

    MsgText.append(QString::number(Slot+1,10));
    MsgText.append(" clicked.\nPress <OK> and set cursor to the password dialog");

    msgBox.setText(MsgText);
    msgBox.exec();

    Sleep::msleep(1000);

    ret_s32 = cryptostick->passwordSafeSendSlotDataViaHID (Slot,PWS_SEND_PASSWORD);
    if (ERR_NO_ERROR != ret_s32)
    {
        QMessageBox msgBox;
        msgBox.setText("Can't send password chars via HID.");
        msgBox.exec();
        return;
    }

    ret_s32 = cryptostick->passwordSafeSendSlotDataViaHID (Slot,PWS_SEND_CR);
    if (ERR_NO_ERROR != ret_s32)
    {
        QMessageBox msgBox;
        msgBox.setText("Can't send CR via HID");
        msgBox.exec();
        return;
    }
*/
}

void MainWindow::PWS_Clicked_Slot00 () { PWS_ExceClickedSlot ( 0); }
void MainWindow::PWS_Clicked_Slot01 () { PWS_ExceClickedSlot ( 1); }
void MainWindow::PWS_Clicked_Slot02 () { PWS_ExceClickedSlot ( 2); }
void MainWindow::PWS_Clicked_Slot03 () { PWS_ExceClickedSlot ( 3); }
void MainWindow::PWS_Clicked_Slot04 () { PWS_ExceClickedSlot ( 4); }
void MainWindow::PWS_Clicked_Slot05 () { PWS_ExceClickedSlot ( 5); }
void MainWindow::PWS_Clicked_Slot06 () { PWS_ExceClickedSlot ( 6); }
void MainWindow::PWS_Clicked_Slot07 () { PWS_ExceClickedSlot ( 7); }
void MainWindow::PWS_Clicked_Slot08 () { PWS_ExceClickedSlot ( 8); }
void MainWindow::PWS_Clicked_Slot09 () { PWS_ExceClickedSlot ( 9); }
void MainWindow::PWS_Clicked_Slot10 () { PWS_ExceClickedSlot (10); }
void MainWindow::PWS_Clicked_Slot11 () { PWS_ExceClickedSlot (11); }
void MainWindow::PWS_Clicked_Slot12 () { PWS_ExceClickedSlot (12); }
void MainWindow::PWS_Clicked_Slot13 () { PWS_ExceClickedSlot (13); }
void MainWindow::PWS_Clicked_Slot14 () { PWS_ExceClickedSlot (14); }
void MainWindow::PWS_Clicked_Slot15 () { PWS_ExceClickedSlot (15); }

/*******************************************************************************

  resetTime

  Reviews
  Date      Reviewer        Info
  27.07.14  SN              First review

*******************************************************************************/

void MainWindow::resetTime()
{

    bool ok;

    if (!cryptostick->validPassword){
    
        do {
            PinDialog dialog("Enter card admin PIN", "Admin PIN:", cryptostick, PinDialog::PLAIN, PinDialog::ADMIN_PIN);
            ok = dialog.exec();
            QString password;
            dialog.getPassword(password);

            if (QDialog::Accepted == ok)
            {
                uint8_t tempPassword[25];
                for (int i=0;i<25;i++)
                    tempPassword[i]=qrand()&0xFF;

                cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(),tempPassword);
                if (cryptostick->validPassword){
                    lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
                } else {
                    csApplet->warningBox("Wrong Pin. Please try again.");
                }
                password.clear();
            }
        } while ( QDialog::Accepted == ok && !cryptostick->validPassword);
    }

// Start the config dialog
    if (cryptostick->validPassword){
        cryptostick->setTime(TOTP_SET_TIME);
   }
}

/*******************************************************************************

  getNextCode

  Reviews
  Date      Reviewer        Info
  01.08.14  SN              First review

*******************************************************************************/

int MainWindow::getNextCode(uint8_t slotNumber)
{
    uint8_t result[18];
    memset(result,0,18);
    uint32_t code;
    uint8_t config;
    int ret;
    bool ok;

    uint16_t lastInterval = 30;

    if (lastInterval<1)
        lastInterval=1;

    if(cryptostick->otpPasswordConfig[0] == 1)
    {
        if (!cryptostick->validUserPassword)
        {
            cryptostick->getUserPasswordRetryCount();

            PinDialog dialog("Enter card user PIN", "User PIN:", cryptostick, PinDialog::PLAIN, PinDialog::USER_PIN);
            ok = dialog.exec();
            QString password;
            dialog.getPassword(password);

            if (QDialog::Accepted == ok)
            {
                uint8_t tempPassword[25];

                for (int i=0;i<25;i++)
                    tempPassword[i]=qrand()&0xFF;

                cryptostick->userAuthenticate((uint8_t *)password.toLatin1().data(),tempPassword);

                if (cryptostick->validUserPassword){
                    lastUserAuthenticateTime = QDateTime::currentDateTime().toTime_t();
                }
                password.clear();
            }
            else
            {
                return 1;
            }
        }
    }
// Start the config dialog
    if ((TRUE == cryptostick->validUserPassword) || (cryptostick->otpPasswordConfig[0] != 1))
    {

    if (slotNumber>=0x20)
    cryptostick->TOTPSlots[slotNumber-0x20]->interval = lastInterval;

    QString output;

     lastTOTPTime = QDateTime::currentDateTime().toTime_t();

     ret = cryptostick->setTime(TOTP_CHECK_TIME);

     bool answer;
     if(ret == -2){
         answer = csApplet->detailedYesOrNoBox("Time is out-of-sync",
         "WARNING!\n\nThe time of your computer and Crypto Stick are out of sync.\nYour computer may be configured with a wrong time or\nyour Crypto Stick may have been attacked. If an attacker or\nmalware could have used your Crypto Stick you should reset the secrets of your configured One Time Passwords. If your computer's time is wrong, please configure it correctly and reset the time of your Crypto Stick.\n\nReset Crypto Stick's time?",
         0, false);

         if (answer)
         {
                resetTime();
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                Sleep::msleep(1000);
                QApplication::restoreOverrideCursor();
                generateAllConfigs();
                
                csApplet->messageBox("Time reset!");
          } else {
               return 1;
          }
      }

     cryptostick->getCode(slotNumber,lastTOTPTime/lastInterval,lastTOTPTime,lastInterval,result);

     //cryptostick->getCode(slotNo,1,result);
     code=result[0]+(result[1]<<8)+(result[2]<<16)+(result[3]<<24);
     config=result[4];



     /*if (config&(1<<2))
         output.append(QByteArray((char *)(result+5),12));*/

     if (config&(1<<0)){
             code=code%100000000;
             output.append(QString( "%1" ).arg(QString::number(code),8,'0') );
         }
             else{

         code=code%1000000;
         output.append(QString( "%1" ).arg(QString::number(code),6,'0') );
     }


     qDebug() << "Current time:" << lastTOTPTime;
     qDebug() << "Counter:" << lastTOTPTime/lastInterval;
     qDebug() << "TOTP:" << code;

     //ui->lineEdit->setText(output);
     otpInClipboard = output;
     copyToClipboard(otpInClipboard);
    }
     else if (ok){
          csApplet->warningBox("Invalid password!");
          return 1;
     }

     return 0;

}

/*******************************************************************************

  on_testHOTPButton_clicked()

  Reviews
  Date      Reviewer        Info
  01.08.14  SN              First review

*******************************************************************************/

//START - OTP Test Routine --------------------------------
/*
void MainWindow::on_testHOTPButton_clicked(){

    uint16_t results;
    uint16_t tests_number = ui->testsSpinBox->value();
    uint8_t counter_number = ui->testsSpinBox_2->value();

    results = cryptostick->testHOTP(tests_number,counter_number);

    if(results < 0){
        QMessageBox msgBox;
        msgBox.setText("There was an error with the test. Check if the device is connected and try again.");
        msgBox.exec();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Tested HOTP counter write/read " + QString::number(tests_number) + " times.\nOf those " + QString::number(results) +" were successful");
        msgBox.exec();
    }

}
*/
//END - OTP Test Routine ----------------------------------

/*******************************************************************************

  on_testTOTPButton_clicked()

  Reviews
  Date      Reviewer        Info
  01.08.14  SN              First review

*******************************************************************************/

//START - OTP Test Routine --------------------------------
/*
void MainWindow::on_testTOTPButton_clicked(){

    uint16_t results;
    uint16_t tests_number = ui->testsSpinBox->value();

    results = cryptostick->testTOTP(tests_number);

    if(results < 0){
        QMessageBox msgBox;
        msgBox.setText("There was an error with the test. Check if the device is connected and try again.");
        msgBox.exec();
    } else {
        QMessageBox msgBox;
        msgBox.setText("Tested TOTP counter write/read " + QString::number(tests_number) + " times.\nOf those " + QString::number(results) +" were successful");
        msgBox.exec();
    }

}
*/
//END - OTP Test Routine ----------------------------------

/*******************************************************************************

  on_pushButton_GotoTOTP_clicked

  Changes
  Date      Author        Info
  04.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_pushButton_GotoTOTP_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tab);
    ui->slotComboBox->setCurrentIndex(0);
}

/*******************************************************************************

  on_pushButton_GotoHOTP_clicked

  Changes
  Date      Author        Info
  04.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_pushButton_GotoHOTP_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tab);
    ui->slotComboBox->setCurrentIndex(TOTP_SlotCount+1);
}

/*******************************************************************************

  on_pushButton_StaticPasswords_clicked

  Changes
  Date      Author        Info
  04.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_pushButton_StaticPasswords_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tab_3);
}

/*******************************************************************************

  on_pushButton_GotoGenOTP_clicked

  Changes
  Date      Author        Info
  04.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_pushButton_GotoGenOTP_clicked()
{
    ui->tabWidget->setCurrentWidget(ui->tab_2);
}

/*******************************************************************************

  on_pushButton_GotoGenOTP_clicked

  Changes
  Date      Author        Info
  04.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

#define PWS_RANDOM_PASSWORD_CHAR_SPACE "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"$%&/()=?[]{}~*+#_'-`,.;:><^|@\\"

void MainWindow::on_PWS_ButtonCreatePW_clicked()
{
    int   i;
    int   n;
    int   PasswordCharSpaceLen;
    char  RandomPassword[30];
    char *PasswordCharSpace = PWS_RANDOM_PASSWORD_CHAR_SPACE;
    QString Text;

//    qDebug() << "Password char space is" << PWS_RANDOM_PASSWORD_CHAR_SPACE;


    PasswordCharSpaceLen = strlen (PasswordCharSpace);
//    qDebug() << "PasswordCharSpaceLen " << PasswordCharSpaceLen;

    PWS_CreatePWSize = 20;
    for (i=0;i<PWS_CreatePWSize;i++)
    {
        n = qrand ();
        n = n % PasswordCharSpaceLen;
        RandomPassword[i] = PasswordCharSpace[n];
//        qDebug() << "n " << n << " - " << RandomPassword[i];
    }
    RandomPassword[i] = 0;

    Text = RandomPassword;
    ui->PWS_EditPassword->setText(Text.toLocal8Bit());

/*
// Check password char space
    Text = PWS_RANDOM_PASSWORD_CHAR_SPACE;
    ui->PWS_EditPassword->setMaxLength(100);

// Set new password size
    PWS_CreatePWSize += 4;
    if (20 < PWS_CreatePWSize)
    {
        PWS_CreatePWSize = 12;
    }
    ui->PWS_ButtonCreatePW->setText(QString("Generate random password ").append(QString::number(PWS_CreatePWSize,10).append(QString(" chars"))));
*/
}

/*******************************************************************************

  on_PWS_ButtonEnable_clicked

  Changes
  Date      Author        Info
  16.09.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_PWS_ButtonEnable_clicked()
{
    PWS_Clicked_EnablePWSAccess();
/*
    uint8_t password[LOCAL_PASSWORD_SIZE];
    bool    ret;
    int     ret_s32;

    do {
        PinDialog dialog("Enter user PIN", "User Pin:", cryptostick, PinDialog::PLAIN, PinDialog::USER_PIN);
        ret = dialog.exec();

        if (QDialog::Accepted == ret)
        {
            dialog.getPassword ((char*)password);

            ret_s32 = cryptostick->passwordSafeEnable ((char*)password);
            switch(ret_s32)
            {
                case ERR_NO_ERROR:
                    SetupPasswordSafeConfig ();
                    generateMenu ();
                    break;

                case ERR_STATUS_NOT_OK:
                    csApplet->warningBox(tr("Wrong pin. Please try again").arg(ret_s32));
                    break;

                default:
                    csApplet->warningBox(tr("Can't unlock password safe. (Error %1)").arg(ret_s32));
                    break;
            }
        }
    } while ( (QDialog::Accepted == ret) && (ret_s32 == ERR_STATUS_NOT_OK) );
*/
}


/*******************************************************************************

  on_counterEdit_editingFinished

  Changes
  Date      Author        Info
  01.10.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


void MainWindow::on_counterEdit_editingFinished()
{
    int Seed;

    Seed = ui->counterEdit->text().toInt();

    if ((1 << 20) < Seed)
    {
        Seed = (1 << 20) -1;
        Seed = (Seed / 16) * 16;
        ui->counterEdit->setText (QString ("%1").sprintf("%d",Seed));

        csApplet->warningBox("Seed must be lower than 1048560 (= 2^20)");
    }

    if (0 != (Seed % 16))
    {
        Seed = (Seed / 16) * 16;
        ui->counterEdit->setText (QString ("%1").sprintf("%d",Seed));

        csApplet->warningBox("Seed had to be a multiple of 16");
    }


}

