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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "device.h"
#include "response.h"
#include "string.h"
#include "sleep.h"
#include "base32.h"
#include "passworddialog.h"
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

#include <QTimer>
#include <QMenu>
#include <QDialog>
#include <QtWidgets>
#include <QDateTime>
#include <QThread>

enum DialogCode { Rejected, Accepted };     // Why not found ?

/*******************************************************************************

 External declarations

*******************************************************************************/

//extern "C" char DebugText_Stick20[600000];

extern "C" void DebugAppendText (char *Text);
extern "C" void DebugClearText (void);

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
    int ret;
    QMetaObject::Connection ret_connection;

    HOTP_SlotCount = HOTP_SLOT_COUNT;
    TOTP_SlotCount = TOTP_SLOT_COUNT;

    trayMenu               = NULL;
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

    PWS_Access = FALSE;

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
            trayIcon->showMessage ("Cryptostick GUI","active - DEBUG Mode");
        }
        else
        {
            trayIcon->showMessage ("Cryptostick GUI","active");
        }
    }
    else
    {
        QMessageBox msgBox;
        if (TRUE == DebugWindowActive)
        {
            msgBox.setText("Cryptostick GUI active - DEBUG Mode");
        }
        else
        {
            msgBox.setText("Cryptostick GUI active");
        }
        msgBox.exec();
    }



    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    restoreAction = new QAction(tr("&Configure"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(startConfiguration()));

    DebugAction = new QAction(tr("&Debug"), this);
    connect(DebugAction, SIGNAL(triggered()), this, SLOT(startStickDebug()));

    ActionAboutDialog = new QAction(tr("&About Crypto Stick"), this);
    connect(ActionAboutDialog, SIGNAL(triggered()), this, SLOT(startAboutDialog()));

    initActionsForStick20 ();

    connect(ui->secretEdit, SIGNAL(textEdited(QString)), this, SLOT(checkTextEdited()));

    // Init debug text
    DebugClearText ();
    DebugAppendText ((char *)"Start Debug - ");

#ifdef WIN32
    DebugAppendText ((char *)"WIN32 system\n");
#endif

#ifdef linux
    DebugAppendText ((char *)"LINUX system\n");
#endif

#ifdef MAC
    DebugAppendText ((char *)"MAC system\n");
#endif

    {
        union {
            unsigned char input[4];
            unsigned int  endianCheck;
        } uEndianCheck;

        unsigned char text[50];

        DebugAppendText ((char *)"\nEndian check\n\n");

        DebugAppendText ((char *)"Store 0x01 0x02 0x03 0x04 in memory locations x,x+1,x+2,x+3\n");
        DebugAppendText ((char *)"then read the location x - x+3 as an unsigned int\n\n");

        uEndianCheck.input[0] = 0x01;
        uEndianCheck.input[1] = 0x02;
        uEndianCheck.input[2] = 0x03;
        uEndianCheck.input[3] = 0x04;

        sprintf ((char*)text,"write u8  %02x%02x%02x%02x\n",uEndianCheck.input[0],uEndianCheck.input[1],uEndianCheck.input[2],uEndianCheck.input[3]);
        DebugAppendText ((char*)text);

        sprintf ((char*)text,"read  u32 %08x\n",uEndianCheck.endianCheck);
        DebugAppendText ((char*)text);

        DebugAppendText ((char *)"\n");

        if (0x01020304 == uEndianCheck.endianCheck)
        {
            DebugAppendText ((char *)"System is little endian\n");
        }
        if (0x04030201 == uEndianCheck.endianCheck)
        {
            DebugAppendText ((char *)"System is big endian\n");
        }
        DebugAppendText ((char *)"\n");
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

        strcpy ((char*)password,"P123456");

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
        break;
    case QSystemTrayIcon::Trigger:
        trayMenu->popup(QCursor::pos());
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
            && (0x03 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8)     // ScanDisk
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

        fp = fopen (LogFile,"a");
        if (NULL != fp)
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

    DebugAppendText ((char *)"Production Infos\n");

    sprintf ((char*)text,"Firmware     %d.%d\n",Stick20ProductionInfos_st.FirmwareVersion_au8[0],Stick20ProductionInfos_st.FirmwareVersion_au8[1]);
    DebugAppendText ((char*)text);
    sprintf ((char*)text,"CPU ID       0x%08lx\n",Stick20ProductionInfos_st.CPU_CardID_u32);
    DebugAppendText ((char*)text);
    sprintf ((char*)text,"Smartcard ID 0x%08lx\n",Stick20ProductionInfos_st.SmartCardID_u32);
    DebugAppendText ((char*)text);
    sprintf ((char*)text,"SD card ID   0x%08lx\n",Stick20ProductionInfos_st.SD_CardID_u32);
    DebugAppendText ((char*)text);


    DebugAppendText ((char*)"Password retry count\n");
    sprintf ((char*)text,"Admin        %d\n",Stick20ProductionInfos_st.SC_AdminPwRetryCount);
    DebugAppendText ((char*)text);
    sprintf ((char*)text,"User         %d\n",Stick20ProductionInfos_st.SC_UserPwRetryCount);
    DebugAppendText ((char*)text);

    DebugAppendText ((char*)"SD card infos\n");
    sprintf ((char*)text,"Manufacturer 0x%02x\n",Stick20ProductionInfos_st.SD_Card_Manufacturer_u8);
    DebugAppendText ((char*)text);
    sprintf ((char*)text,"OEM          0x%04x\n",Stick20ProductionInfos_st.SD_Card_OEM_u16);
    DebugAppendText ((char*)text);
    sprintf ((char*)text,"Manufa. date %d.%02d\n",Stick20ProductionInfos_st.SD_Card_ManufacturingYear_u8+2000,Stick20ProductionInfos_st.SD_Card_ManufacturingMonth_u8);
    DebugAppendText ((char*)text);
    sprintf ((char*)text,"Write speed  %d kB/sec\n",Stick20ProductionInfos_st.SD_WriteSpeed_u16);
    DebugAppendText ((char*)text);

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
            ui->statusBar->showMessage("Device connected.");

            if(set_initial_time == FALSE) {
                ret = cryptostick->setTime(TOTP_CHECK_TIME);
                set_initial_time = TRUE;
            } else {
                ret = 0;
            }

            if(ret == -2){
                 QMessageBox msgBox;
                 msgBox.setWindowTitle("Time is out-of-sync");
                 msgBox.setText("WARNING!\n\nThe time of your computer and Crypto Stick are out of sync. Your computer may be configured with a wrong time or your Crypto Stick may have been attacked. If an attacker or malware could have used your Crypto Stick you should reset the secrets of your configured One Time Passwords. If your computer's time is wrong, please configure it correctly and reset the time of your Crypto Stick.\n\nReset Crypto Stick's time?");
                 msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                 msgBox.setDefaultButton(QMessageBox::No);
                 ret = msgBox.exec();

                 switch (ret) {
                   case QMessageBox::Yes:
                        resetTime();
                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        Sleep::msleep(1000);
                        QApplication::restoreOverrideCursor();
                        generateAllConfigs();

                        msgBox.setText("Time reset!");
                        msgBox.setStandardButtons(QMessageBox::Ok);
                        msgBox.exec();

                       break;
                   case QMessageBox::No:

                       break;
                   default:
                       // should never be reached
                       break;
                 }
             }

            cryptostick->getStatus();
        } else
        {
            ui->statusBar->showMessage("Device Stick 2.0 connected.");
        }
        DeviceOffline = FALSE;
    }
    else if (result == -1)
    {
        ui->statusBar->showMessage("Device disconnected.");
        CryptedVolumeActive = FALSE;
        HiddenVolumeActive  = FALSE;
        set_initial_time = FALSE;
        if (FALSE== DeviceOffline)      // To avoid the continuous reseting of the menu
        {
            generateMenu();
            DeviceOffline = TRUE;
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
                msgBox.setText("Flash Cryptostick application");
                msgBox.exec();
            }
        }
*/
    }
    else if (result == 1){ //recreate the settings and menus
        if (false == cryptostick->activStick20)
        {
            ui->statusBar->showMessage("Device connected.");
            if(set_initial_time == FALSE){
                ret = cryptostick->setTime(TOTP_CHECK_TIME);
                set_initial_time = TRUE;
            } else {
                ret = 0;
            }

            if(ret == -2){
                 QMessageBox msgBox;
                 msgBox.setWindowTitle("Time is out-of-sync");
                 msgBox.setText("WARNING!\n\nThe time of your computer and Crypto Stick are out of sync. Your computer may be configured with a wrong time or your Crypto Stick may have been attacked. If an attacker or malware could have used your Crypto Stick you should reset the secrets of your configured One Time Passwords. If your computer's time is wrong, please configure it correctly and reset the time of your Crypto Stick.\n\nReset Crypto Stick's time?");
                 msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                 msgBox.setDefaultButton(QMessageBox::No);
                 ret = msgBox.exec();

                 switch (ret) {
                   case QMessageBox::Yes:
                        resetTime();
                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        Sleep::msleep(1000);
                        QApplication::restoreOverrideCursor();
                        generateAllConfigs();

                        msgBox.setText("Time reset!");
                        msgBox.setStandardButtons(QMessageBox::Ok);
                        msgBox.exec();

                       break;
                   case QMessageBox::No:

                       break;
                   default:
                       // should never be reached
                       break;
                 }
             }

            cryptostick->getStatus();
        } else
        {
            ui->statusBar->showMessage("Device Stick 2.0 connected.");
        }
        generateMenu();
    }

    if (TRUE == Stick20_ConfigurationChanged)
    {
        Stick20_ConfigurationChanged = FALSE;

        cryptostick->userPasswordRetryCount = HID_Stick20Configuration_st.UserPwRetryCount;
        cryptostick->passwordRetryCount     = HID_Stick20Configuration_st.AdminPwRetryCount;

        UpdateDynamicMenuEntrys ();

        if (TRUE == StickNotInitated)
        {
            if (FALSE == StickNotInitated_DontAsk)
            {
                QMessageBox msgBox;
                //int ret;

                msgBox.setText("Warning: Encrypted Volume not secure\nSelect -Init encrypted volume-");
//                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
//                msgBox.setDefaultButton(QMessageBox::Yes);
                ret = msgBox.exec();
            }
        }
        if (TRUE == SdCardNotErased)
        {
            if (FALSE == SdCardNotErased_DontAsk)
            {
                QMessageBox msgBox;
                //int ret;

                msgBox.setText("Warning: Encrypted Volume not secure\nSelect -Fill encrypted volume with ramdom chars-");
                ret = msgBox.exec();
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
    connect(Stick20ActionEnableCryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20EnableCryptedVolume()));

    Stick20ActionDisableCryptedVolume = new QAction(tr("&Lock encrypted volume"), this);
    connect(Stick20ActionDisableCryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20DisableCryptedVolume()));

    Stick20ActionEnableHiddenVolume = new QAction(tr("&Unlock hidden volume"), this);
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

    Stick20ActionDestroyCryptedVolume = new QAction(tr("&Destroy encrypted volume"), this);
    connect(Stick20ActionDestroyCryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20DestroyCryptedVolume()));

    Stick20ActionInitCryptedVolume = new QAction(tr("&Init encrypted volume"), this);
    connect(Stick20ActionInitCryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20DestroyCryptedVolume()));

    Stick20ActionFillSDCardWithRandomChars = new QAction(tr("&Initialize storage with random data"), this);
    connect(Stick20ActionFillSDCardWithRandomChars, SIGNAL(triggered()), this, SLOT(startStick20FillSDCardWithRandomChars()));

    Stick20ActionGetStickStatus = new QAction(tr("&Get stick status"), this);
    connect(Stick20ActionGetStickStatus, SIGNAL(triggered()), this, SLOT(startStick20GetStickStatus()));
//    connect(Stick20ActionGetStickStatus, SIGNAL(triggered()), this, SLOT(startAboutDialog()));

    Stick20ActionSetReadonlyUncryptedVolume = new QAction(tr("&Set readonly unencrypted volume"), this);
    connect(Stick20ActionSetReadonlyUncryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20SetReadonlyUncryptedVolume()));

    Stick20ActionSetReadWriteUncryptedVolume = new QAction(tr("&Set readwrite unencrypted volume"), this);
    connect(Stick20ActionSetReadWriteUncryptedVolume, SIGNAL(triggered()), this, SLOT(startStick20SetReadWriteUncryptedVolume()));

    Stick20ActionDebugAction = new QAction(tr("&Debug Action"), this);
    connect(Stick20ActionDebugAction, SIGNAL(triggered()), this, SLOT(startStick20DebugAction()));

    Stick20ActionSetupHiddenVolume = new QAction(tr("&Setup hidden volume"), this);
    connect(Stick20ActionSetupHiddenVolume, SIGNAL(triggered()), this, SLOT(startStick20SetupHiddenVolume()));

    Stick20ActionClearNewSDCardFound = new QAction(tr("&Clear - Initialize storage with random data"), this);
    connect(Stick20ActionClearNewSDCardFound, SIGNAL(triggered()), this, SLOT(startStick20ClearNewSdCardFound()));

    Stick20ActionSetupPasswordMatrix = new QAction(tr("&Setup password matrix"), this);
    connect(Stick20ActionSetupPasswordMatrix, SIGNAL(triggered()), this, SLOT(startStick20SetupPasswordMatrix()));

    Stick20ActionLockStickHardware = new QAction(tr("&Lock stick hardware"), this);
    connect(Stick20ActionLockStickHardware, SIGNAL(triggered()), this, SLOT(startStick20LockStickHardware()));

    Stick20ActionResetUserPassword = new QAction(tr("&Reset user password"), this);
    connect(Stick20ActionResetUserPassword, SIGNAL(triggered()), this, SLOT(startResetUserPassword()));


}

/*******************************************************************************

  generateMenuOTP

  Changes
  Date      Author        Info
  24.03.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::generateMenuOTP()
{

    if (cryptostick->TOTPSlots[0]->isProgrammed==true){
        QString actionName("TOTP slot 1 ");
        trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[0]->slotName),this,SLOT(getTOTP1()));
    }
    if (cryptostick->TOTPSlots[1]->isProgrammed==true){
        QString actionName("TOTP slot 2 ");
        trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[1]->slotName),this,SLOT(getTOTP2()));
    }
    if (cryptostick->TOTPSlots[2]->isProgrammed==true){
        QString actionName("TOTP slot 3 ");
        trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[2]->slotName),this,SLOT(getTOTP3()));
    }
    if (cryptostick->TOTPSlots[3]->isProgrammed==true){
        QString actionName("TOTP slot 4 ");
        trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[3]->slotName),this,SLOT(getTOTP4()));
    }

    if (TOTP_SlotCount > 4)
    {
        if (cryptostick->TOTPSlots[4]->isProgrammed==true){
            QString actionName("TOTP slot 5 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[4]->slotName),this,SLOT(getTOTP5()));
        }
    }
    if (TOTP_SlotCount > 5)
    {
        if (cryptostick->TOTPSlots[5]->isProgrammed==true){
            QString actionName("TOTP slot 6 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[5]->slotName),this,SLOT(getTOTP6()));
        }
    }
    if (TOTP_SlotCount > 6)
    {
        if (cryptostick->TOTPSlots[6]->isProgrammed==true){
            QString actionName("TOTP slot 7 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[6]->slotName),this,SLOT(getTOTP7()));
        }
    }
    if (TOTP_SlotCount > 7)
    {
        if (cryptostick->TOTPSlots[7]->isProgrammed==true){
            QString actionName("TOTP slot 8 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[7]->slotName),this,SLOT(getTOTP8()));
        }
    }
    if (TOTP_SlotCount > 8)
    {
        if (cryptostick->TOTPSlots[8]->isProgrammed==true){
            QString actionName("TOTP slot 9 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[8]->slotName),this,SLOT(getTOTP9()));
        }
    }
    if (TOTP_SlotCount > 9)
    {
        if (cryptostick->TOTPSlots[9]->isProgrammed==true){
            QString actionName("TOTP slot 10 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[8]->slotName),this,SLOT(getTOTP10()));
        }
    }
    if (TOTP_SlotCount > 10)
    {
        if (cryptostick->TOTPSlots[10]->isProgrammed==true){
            QString actionName("TOTP slot 11 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[10]->slotName),this,SLOT(getTOTP11()));
        }
    }
    if (TOTP_SlotCount > 11)
    {
        if (cryptostick->TOTPSlots[11]->isProgrammed==true){
            QString actionName("TOTP slot 12 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[11]->slotName),this,SLOT(getTOTP12()));
        }
    }
    if (TOTP_SlotCount > 12)
    {
        if (cryptostick->TOTPSlots[12]->isProgrammed==true){
            QString actionName("TOTP slot 13 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[12]->slotName),this,SLOT(getTOTP13()));
        }
    }
    if (TOTP_SlotCount > 13)
    {
        if (cryptostick->TOTPSlots[13]->isProgrammed==true){
            QString actionName("TOTP slot 14 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[13]->slotName),this,SLOT(getTOTP14()));
        }
    }
    if (TOTP_SlotCount > 14)
    {
        if (cryptostick->TOTPSlots[14]->isProgrammed==true){
            QString actionName("TOTP slot 15 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->TOTPSlots[14]->slotName),this,SLOT(getTOTP15()));
        }
    }

    if (cryptostick->HOTPSlots[0]->isProgrammed==true){
        QString actionName("HOTP slot 1 ");
        trayMenu->addAction(actionName.append((char *)cryptostick->HOTPSlots[0]->slotName),this,SLOT(getHOTP1()));
    }
    if (cryptostick->HOTPSlots[1]->isProgrammed==true){
        QString actionName("HOTP slot 2 ");
        trayMenu->addAction(actionName.append((char *)cryptostick->HOTPSlots[1]->slotName),this,SLOT(getHOTP2()));
    }
    if (HOTP_SlotCount >= 3)
    {
        if (cryptostick->HOTPSlots[2]->isProgrammed==true){
            QString actionName("HOTP slot 3 ");
            trayMenu->addAction(actionName.append((char *)cryptostick->HOTPSlots[2]->slotName),this,SLOT(getHOTP3()));
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
    ui->tabWidget->removeTab(2);

    generateMenuOTP ();

    trayMenu->addAction(restoreAction);
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

// Enable tab for password safe for stick 2
    if (-1 == ui->tabWidget->indexOf (ui->tab_3))
    {
        ui->tabWidget->addTab(ui->tab_3,"Password safe config");
    }

// Setup entrys for password safe
    generateMenuPasswordSafe ();
    trayMenu->addSeparator();

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
        trayMenu->addAction(Stick20ActionClearNewSDCardFound  );
        AddSeperator = TRUE;
    }

    if (TRUE == AddSeperator)
    {
        trayMenu->addSeparator();
    }

    generateMenuOTP ();

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


    trayMenuSubConfigure  = trayMenu->addMenu( "Configure" );
    trayMenuSubConfigure->addAction(restoreAction);
    trayMenuSubConfigure->addAction(Stick20ActionChangeUserPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeAdminPIN);
    trayMenuSubConfigure->addAction(Stick20ActionSetupHiddenVolume);

    if (TRUE == MatrixInputActive)
    {
        trayMenuSubConfigure->addAction(Stick20ActionSetupPasswordMatrix);
    }

    trayMenuSubConfigure->addAction(Stick20ActionDestroyCryptedVolume       );
    trayMenuSubConfigure->addAction(Stick20ActionGetStickStatus             );


    if (TRUE == LockHardware)
    {
        trayMenuSubConfigure->addAction(Stick20ActionLockStickHardware);
    }

    if (TRUE == HiddenVolumeAccessable)
    {

    }

    if (TRUE == ExtendedConfigActive)
    {
        trayMenuSubSpecialConfigure = trayMenuSubConfigure->addMenu( "Special Configure" );
        trayMenuSubSpecialConfigure->addAction(Stick20ActionEnableFirmwareUpdate       );
        trayMenuSubSpecialConfigure->addAction(Stick20ActionExportFirmwareToFile       );
        trayMenuSubSpecialConfigure->addAction(Stick20ActionFillSDCardWithRandomChars  );

        if (FALSE == NormalVolumeRWActive)
        {
            trayMenuSubSpecialConfigure->addAction(Stick20ActionSetReadonlyUncryptedVolume );      // Set RW active
        }
        else
        {
            trayMenuSubSpecialConfigure->addAction(Stick20ActionSetReadWriteUncryptedVolume);      // Set readonly active
        }
    }


// Enable "reset user password" ?
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
        trayMenu->addAction(Stick20Action);
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

        slot->tokenID[12]=ui->keyboardComboBox->currentIndex()&0xFF;

        QByteArray counterFromGUI = QByteArray(ui->counterEdit->text().toLatin1());
        memset(slot->counter,0,8);
        if(!counterFromGUI.length()) {
            memcpy(slot->counter,counterFromGUI.data(),counterFromGUI.length());
        }

        qDebug() << "test";
        qDebug() << slot->counter;
        qDebug() << counterFromGUI.length();

        slot->config=0;

        if (ui->digits8radioButton->isChecked())
            slot->config+=(1<<0);
        if (ui->enterCheckBox->isChecked())
            slot->config+=(1<<1);
        if (ui->tokenIDCheckBox->isChecked())
            slot->config+=(1<<2);      

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

    if(slotNo > TOTP_SlotCount){
        slotNo -= (TOTP_SlotCount + 1);

    } else {
        slotNo += HOTP_SlotCount;
    }

    if (slotNo < HOTP_SlotCount)
    {
        //ui->hotpGroupBox->show();
        ui->hotpGroupBox->setTitle("Parameters");
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

        //slotNo=slotNo+0x10;
        ui->nameEdit->setText(QString((char *)cryptostick->HOTPSlots[slotNo]->slotName));

        QByteArray secret((char *) cryptostick->HOTPSlots[slotNo]->secret,20);
        ui->base32RadioButton->setChecked(true);
        ui->secretEdit->setText(secret);//.toHex());

        QByteArray counter((char *) cryptostick->HOTPSlots[slotNo]->counter,8);
        qDebug() << (char *) cryptostick->HOTPSlots[slotNo]->counter;
        ui->counterEdit->setText(counter.trimmed());//.toHex());

        if (cryptostick->HOTPSlots[slotNo]->counter==0)
            ui->counterEdit->setText("0");

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
    else if ((slotNo >= HOTP_SlotCount) && (slotNo < HOTP_SlotCount + TOTP_SlotCount))
    {
        slotNo -= HOTP_SlotCount;
        //ui->hotpGroupBox->hide();
        ui->hotpGroupBox->setTitle("Parameters");
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
        cryptostick->getPasswordRetryCount();

        QString password = QInputDialog::getText(this, tr("Enter card admin password"),tr("Admin password: ")+tr("(Tries left: ")+QString::number(cryptostick->passwordRetryCount)+")", QLineEdit::Password,"", &ok);

        if (TRUE == ok)
        {
            uint8_t tempPassword[25];

            if ((0 == strcmp (password.toLatin1().data(), "123456")) || (0 == strcmp (password.toLatin1().data(), "12345678")))
            {
                QMessageBox   msgBox;
                msgBox.setText("Warning: Default PIN is used.\nPlease change the PIN");
                msgBox.exec();
            }

            for (int i=0;i<25;i++)
                tempPassword[i]=qrand()&0xFF;

            cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(),tempPassword);
            if (cryptostick->validPassword){
                lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
            }
            password.clear();
        }
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
    else if (ok){
        QMessageBox msgBox;
         msgBox.setText("Invalid password!");
         msgBox.exec();
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

    if (true == cryptostick->activStick20)
    {
    // Get actual data from stick 20
        cryptostick->stick20GetStatusData ();

    // Wait for response
        Stick20ResponseDialog ResponseDialog(this);

        ResponseDialog.NoStopWhenStatusOK ();
        ResponseDialog.NoShowDialog();
        ResponseDialog.hide();

        ResponseDialog.cryptostick=cryptostick;
        ResponseDialog.exec();
        ResponseDialog.ResultValue;
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
    uint8_t password[40];
    bool           ret;

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter user PIN",HID_Stick20Configuration_st.UserPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
    {
//        password[0] = 'P';
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
    uint8_t password[40];

    password[0] = 0;
    stick20SendCommand (STICK20_CMD_DISABLE_CRYPTED_PARI,password);
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
    uint8_t password[40];
    bool    ret;

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter password for hidden volume",-1);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
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
    uint8_t password[40];

    password[0] = 0;
    stick20SendCommand (STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI,password);

}


/*******************************************************************************

  startStick20EnableCryptedVolume

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::startStick20EnableFirmwareUpdate()
{
    uint8_t password[40];
    bool    ret;

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter admin PIN",HID_Stick20Configuration_st.AdminPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
    {
//        password[0] = 'P';
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
    uint8_t password[40];
    bool    ret;

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter admin PIN",HID_Stick20Configuration_st.AdminPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
    {
//        password[0] = 'P';
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_EXPORT_FIRMWARE_TO_FILE,password);
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
    uint8_t password[40];
    bool    ret;

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter admin PIN",HID_Stick20Configuration_st.AdminPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
    {
//        password[0] = 'P';
        dialog.getPassword ((char*)password);

        stick20SendCommand (STICK20_CMD_GENERATE_NEW_KEYS,password);
    }

}

/*******************************************************************************

  startStick20EnableCryptedVolume

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

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter admin PIN",HID_Stick20Configuration_st.AdminPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
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
    uint8_t password[40];
    bool    ret;

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter admin PIN",HID_Stick20Configuration_st.AdminPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
    {
//        password[0] = 'P';
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
    Stick20ResponseDialog ResponseDialog(this);

    ResponseDialog.NoStopWhenStatusOK ();
    ResponseDialog.NoShowDialog();
    ResponseDialog.hide();

    ResponseDialog.cryptostick = cryptostick;
    ResponseDialog.exec();
    ResponseDialog.ResultValue;

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
    uint8_t password[40];
    bool    ret;

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter user PIN",HID_Stick20Configuration_st.UserPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
    {
//        password[0] = 'P';
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
    uint8_t password[40];
    bool    ret;

    PasswordDialog dialog(MatrixInputActive,this);
    dialog.init((char *)"Enter user PIN",HID_Stick20Configuration_st.UserPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
    {
//        password[0] = 'P';
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
    uint8_t password[40];
    bool    ret;

    stick20LockFirmwareDialog dialog(this);

    ret = dialog.exec();
    if (Accepted == ret)
    {
        PasswordDialog dialog1(MatrixInputActive,this);
        dialog1.init((char *)"Enter admin PIN",HID_Stick20Configuration_st.AdminPwRetryCount);
        dialog1.cryptostick = cryptostick;

        ret = dialog1.exec();

        if (Accepted == ret)
        {
            dialog1.getPassword ((char*)password);
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
    QMessageBox          msgBox;
    MatrixPasswordDialog dialog(this);

    msgBox.setText("The selected lines must be greater then greatest password length");
    msgBox.exec();

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

    if (0 == (HID_Stick20Configuration_st.SDFillWithRandomChars_u8 & 0x01))
    {
        SdCardNotErased  = TRUE;
    }
    else
    {
        SdCardNotErased  = FALSE;
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
    QMessageBox msgBox;

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
            {
                UpdateDialog dialog(this);
                ret = dialog.exec();
                if (Accepted == ret)
                {
                    ret = cryptostick->stick20EnableFirmwareUpdate (password);
                    if (TRUE == ret)
                    {
                        waitForAnswerFromStick20 = TRUE;
                    }
                }
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
            {
                msgBox.setText("The generation of new AES keys will destroy the encrypted volume!");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
                ret = msgBox.exec();
                if (Accepted == ret)
                {
                    ret = cryptostick->stick20CreateNewKeys (password);
                    if (TRUE == ret)
                    {
                        waitForAnswerFromStick20 = TRUE;
                    }
                }
            }
            break;
        case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS :
            {
                msgBox.setText("This command fills the encrypted volumes with random data.\nThis will destroy all encrypted volumes!\nThis commands last very long > 1 hour for 32GB");
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setDefaultButton(QMessageBox::No);
                ret = msgBox.exec();
                if (QMessageBox::Yes == ret)
                {
                    ret = cryptostick->stick20FillSDCardWithRandomChars (password,STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ENCRYPTED_VOL);
                    if (TRUE == ret)
                    {
                        waitForAnswerFromStick20    = TRUE;
                        stopWhenStatusOKFromStick20 = TRUE;
                    }
                }
            }
            break;
         case STICK20_CMD_WRITE_STATUS_DATA        :
            msgBox.setText("Not implemented");
            ret = msgBox.exec();
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
            msgBox.setText("Stick20Dialog: Wrong combobox value! ");
            msgBox.exec();
            break;

    }

    Result = FALSE;
    if (TRUE == waitForAnswerFromStick20)
    {
        Stick20ResponseDialog ResponseDialog(this);

        if (FALSE == stopWhenStatusOKFromStick20)
        {
            ResponseDialog.NoStopWhenStatusOK ();
        }
        ResponseDialog.cryptostick=cryptostick;

        ResponseDialog.exec();
        Result = ResponseDialog.ResultValue;
    }

    if (TRUE == Result)
    {
        switch (stick20Command)
        {
            case STICK20_CMD_ENABLE_CRYPTED_PARI            :
                CryptedVolumeActive = TRUE;
                HiddenVolumeActive  = FALSE;
                generateMenu();
                break;
            case STICK20_CMD_DISABLE_CRYPTED_PARI           :
                CryptedVolumeActive = FALSE;
                HiddenVolumeActive  = FALSE;
                generateMenu();
                break;
            case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI     :
                HiddenVolumeActive  = TRUE;
                CryptedVolumeActive = FALSE;
                generateMenu();
                break;
            case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI    :
                CryptedVolumeActive = FALSE;
                HiddenVolumeActive  = FALSE;
                generateMenu();
                break;
            case STICK20_CMD_GET_DEVICE_STATUS              :
                UpdateDynamicMenuEntrys ();
                {
//                    Stick20InfoDialog InfoDialog(this);
//                   InfoDialog.exec();
                }
                break;
            case STICK20_CMD_SEND_STARTUP                   :
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
               NormalVolumeRWActive = TRUE;
               break;
            case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
               NormalVolumeRWActive = FALSE;
               break;
            case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND        :
               HID_Stick20Configuration_st.SDFillWithRandomChars_u8 |= 0x01;
               UpdateDynamicMenuEntrys ();
               break;
            case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS :
                HID_Stick20Configuration_st.SDFillWithRandomChars_u8 |= 0x01;
                UpdateDynamicMenuEntrys ();
                break;
            case STICK20_CMD_GENERATE_NEW_KEYS              :
                HID_Stick20Configuration_st.StickKeysNotInitiated = FALSE;
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
    QMessageBox msgBox;
    int res;

    uint8_t slotNo = ui->slotComboBox->currentIndex();
    if(slotNo > TOTP_SlotCount){
        slotNo -= (TOTP_SlotCount + 1);
    } else {
        slotNo += HOTP_SlotCount;
    }
    if (cryptostick->isConnected){

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
            msgBox.setText("Config written successfully!");
        else if (res == -3)
            msgBox.setText("The name of the slot must not be empty!");
        else
            msgBox.setText("Error writing config!");

        msgBox.exec();


        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        Sleep::msleep(500);
        QApplication::restoreOverrideCursor();

        generateAllConfigs();

    }
    else{
        msgBox.setText("Crypto stick not connected!");
        msgBox.exec();
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
    quint64 counter=qrand();
    counter<<=16;
    counter+=qrand();
    //counter<<=16;
    //counter+=qrand();
    //counter<<=16;
    //counter+=qrand();
    //qDebug() << counter;
    ui->counterEdit->setText(QString(QByteArray::number(counter,8)));
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
    QMessageBox msgBox;
    int res;
    uint8_t data[5];

    if (cryptostick->isConnected){

        data[0]=ui->numLockComboBox->currentIndex()-1;
        data[1]=ui->capsLockComboBox->currentIndex()-1;
        data[2]=ui->scrollLockComboBox->currentIndex()-1;
        if(ui->enableUserPasswordCheckBox->isChecked()){
            data[3]=1;
            if(ui->deleteUserPasswordCheckBox->isChecked()){
                data[4]=1;
            } else {
                data[4]=0;
            }
        } else {
            data[3]=0;
            data[4]=0;
        }

        res =cryptostick->writeGeneralConfig(data);

        if (res==0)
            msgBox.setText("Config written successfully!");
        else
            msgBox.setText("Error writing config!");

        msgBox.exec();

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        Sleep::msleep(500);
        QApplication::restoreOverrideCursor();
        cryptostick->getStatus();
        generateAllConfigs();

    }
    else{
        msgBox.setText("Crypto stick not connected!");
        msgBox.exec();
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
    //HOTPDialog dialog(this);
    //dialog.device=cryptostick;
    //dialog.slotNumber=0x10 + slot;
    //dialog.title=QString("HOTP slot ").append(QString::number(slot+1,10)).append(" [").append((char *)cryptostick->HOTPSlots[slot]->slotName).append("]");
    //dialog.setToHOTP();
    //dialog.getNextCode();
    //dialog.exec();

    int ret;

    ret = getNextCode(0x10 + slot);
    if(ret == 0){
    if(cryptostick->HOTPSlots[slot]->slotName[0] == '\0')
        trayIcon->showMessage (QString("HOTP slot ").append(QString::number(slot+1,10)),"One-time password has been copied to clipboard.");
    else
        trayIcon->showMessage (QString("HOTP slot ").append(QString::number(slot+1,10)).append(" [").append((char *)cryptostick->HOTPSlots[slot]->slotName).append("]"),"One-time password has been copied to clipboard.");
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
        trayIcon->showMessage (QString("TOTP slot ").append(QString::number(slot+1,10)),"One-time password has been copied to clipboard.");
    else
        trayIcon->showMessage (QString("TOTP slot ").append(QString::number(slot+1,10)).append(" [").append((char *)cryptostick->TOTPSlots[slot]->slotName).append("]"),"One-time password has been copied to clipboard.");
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

  getTOTP2

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/
/*
void MainWindow::getTOTP2()
{
    HOTPDialog dialog(this);
    dialog.device=cryptostick;
    dialog.slotNumber=0x21;
    dialog.title=QString("TOTP slot 2 [").append((char *)cryptostick->TOTPSlots[1]->slotName).append("]");
    dialog.setToTOTP();
    dialog.getNextCode();
    dialog.exec();
}
*/
/*******************************************************************************

  getTOTP3

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/
/*
void MainWindow::getTOTP3()
{
    HOTPDialog dialog(this);
    dialog.device=cryptostick;
    dialog.slotNumber=0x22;
    dialog.title=QString("TOTP slot 3 [").append((char *)cryptostick->TOTPSlots[2]->slotName).append("]");
    dialog.setToTOTP();
    dialog.getNextCode();
    dialog.exec();
}
*/
/*******************************************************************************

  getTOTP4

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/
/*
void MainWindow::getTOTP4()
{
    HOTPDialog dialog(this);
    dialog.device=cryptostick;
    dialog.slotNumber=0x23;
    dialog.title=QString("TOTP slot 4 [").append((char *)cryptostick->TOTPSlots[3]->slotName).append("]");
    dialog.setToTOTP();
    dialog.getNextCode();
    dialog.exec();
}
*/



/*******************************************************************************

  on_eraseButton_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MainWindow::on_eraseButton_clicked()
{
    QMessageBox msgBox;
     msgBox.setText("Are you sure you want to erase the slot?");
     msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
     msgBox.setDefaultButton(QMessageBox::No);
     int ret = msgBox.exec();
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

     switch (ret) {
       case QMessageBox::Yes:
            cryptostick->eraseSlot(slotNo);
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            Sleep::msleep(1000);
            QApplication::restoreOverrideCursor();
            generateAllConfigs();

            msgBox.setText("Slot erased!");
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.exec();

           break;
       case QMessageBox::No:

           break;
       default:
           // should never be reached
           break;
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
                    strcpy ((char*)cryptostick->passwordSafeSlotNames[i],(char*)cryptostick->passwordSafeSlotName);
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
        ui->PWS_ComboBoxSelectSlot->addItem(QString("*** No slot access ***"));
    }

    ui->PWS_EditSlotName->setMaxLength(PWS_SLOTNAME_LENGTH);
    ui->PWS_EditPassword->setMaxLength(PWS_PASSWORD_LENGTH);
    ui->PWS_EditLoginName->setMaxLength(PWS_LOGINNAME_LENGTH);

    ui->PWS_CheckBoxHideSecret->setChecked(TRUE);
    ui->PWS_EditPassword->setEchoMode(QLineEdit::Password);

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
    int ret;

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
            QMessageBox msgBox;
            msgBox.setText("Can't clear slot");
            msgBox.exec();
        }
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("Slot already erased");
        msgBox.exec();
    }

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

  on_PWS_ButtonSaveSlot_pressed

  Changes
  Date      Author        Info
  31.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void MainWindow::on_PWS_ButtonSaveSlot_pressed()
{
    int Slot;
    int ret;
    uint8_t SlotName[PWS_SLOTNAME_LENGTH+1];
    uint8_t LoginName[PWS_LOGINNAME_LENGTH+1];
    uint8_t Password[PWS_PASSWORD_LENGTH+1];


    Slot = ui->PWS_ComboBoxSelectSlot->currentIndex();

    strncpy ((char*)SlotName,ui->PWS_EditSlotName->text().toLatin1(),PWS_SLOTNAME_LENGTH);
    SlotName[PWS_SLOTNAME_LENGTH] = 0;

    strncpy ((char*)LoginName,ui->PWS_EditLoginName->text().toLatin1(),PWS_LOGINNAME_LENGTH);
    LoginName[PWS_LOGINNAME_LENGTH] = 0;

    strncpy ((char*)Password,ui->PWS_EditPassword->text().toLatin1(),PWS_PASSWORD_LENGTH);
    Password[PWS_PASSWORD_LENGTH] = 0;

    ret = cryptostick->setPasswordSafeSlotData_1 (Slot,(uint8_t*)SlotName,(uint8_t*)Password);
    if (ERR_NO_ERROR == ret)
    {
        return;
    }

    ret = cryptostick->setPasswordSafeSlotData_2 (Slot,(uint8_t*)LoginName);
    if (ERR_NO_ERROR == ret)
    {
        QMessageBox msgBox;
        msgBox.setText("Can't save slot");
        msgBox.exec();
        return;
    }

    cryptostick->passwordSafeStatus[Slot] = TRUE;
    strcpy ((char*)cryptostick->passwordSafeSlotNames[Slot],(char*)SlotName);
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
        strcpy ((char*)cryptostick->passwordSafeSlotNames[Slot],(char*)cryptostick->passwordSafeSlotName);
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
        QString actionName("Enable password safe");
        trayMenu->addAction(actionName,this,SLOT(PWS_Clicked_EnablePWSAccess()));
        return;
    }

    if (cryptostick->passwordSafeStatus[0] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (0)),this,SLOT(PWS_Clicked_Slot00()));
    }
    if (cryptostick->passwordSafeStatus[1] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (1)),this,SLOT(PWS_Clicked_Slot01()));
    }
    if (cryptostick->passwordSafeStatus[2] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (2)),this,SLOT(PWS_Clicked_Slot02()));
    }
    if (cryptostick->passwordSafeStatus[3] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (3)),this,SLOT(PWS_Clicked_Slot03()));
    }
    if (cryptostick->passwordSafeStatus[4] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (4)),this,SLOT(PWS_Clicked_Slot04()));
    }
    if (cryptostick->passwordSafeStatus[5] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (5)),this,SLOT(PWS_Clicked_Slot05()));
    }
    if (cryptostick->passwordSafeStatus[6] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (6)),this,SLOT(PWS_Clicked_Slot06()));
    }
    if (cryptostick->passwordSafeStatus[7] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (7)),this,SLOT(PWS_Clicked_Slot07()));
    }
    if (cryptostick->passwordSafeStatus[8] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (8)),this,SLOT(PWS_Clicked_Slot08()));
    }
    if (cryptostick->passwordSafeStatus[9] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (9)),this,SLOT(PWS_Clicked_Slot09()));
    }
    if (cryptostick->passwordSafeStatus[10] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (10)),this,SLOT(PWS_Clicked_Slot10()));
    }
    if (cryptostick->passwordSafeStatus[10] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (10)),this,SLOT(PWS_Clicked_Slot10()));
    }
    if (cryptostick->passwordSafeStatus[11] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (11)),this,SLOT(PWS_Clicked_Slot11()));
    }
    if (cryptostick->passwordSafeStatus[12] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (12)),this,SLOT(PWS_Clicked_Slot12()));
    }
    if (cryptostick->passwordSafeStatus[13] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (13)),this,SLOT(PWS_Clicked_Slot13()));
    }
    if (cryptostick->passwordSafeStatus[14] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (14)),this,SLOT(PWS_Clicked_Slot14()));
    }
    if (cryptostick->passwordSafeStatus[15] == (unsigned char)true)
    {
        QString actionName("PW Safe - ");
        trayMenu->addAction(actionName.append(PWS_GetSlotName (15)),this,SLOT(PWS_Clicked_Slot15()));
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
    uint8_t password[40];
    bool    ret;
    int     ret_s32;

    PasswordDialog dialog(FALSE,this);
    dialog.init((char *)"Enter user PIN",HID_Stick20Configuration_st.UserPwRetryCount);
    dialog.cryptostick = cryptostick;

    ret = dialog.exec();

    if (Accepted == ret)
    {
        dialog.getPassword ((char*)password);

        ret_s32 = cryptostick->passwordSafeEnable ((char*)&password[1]);
        if (ERR_NO_ERROR != ret_s32)
        {
            QMessageBox msgBox;
            msgBox.setText("Can't enable password safe");
            msgBox.exec();
        }
        else
        {
            SetupPasswordSafeConfig ();
            generateMenu ();
        }

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

void MainWindow::PWS_ExceClickedSlot (int Slot)
{

    PasswordSafeDialog PWS_dialog (Slot,this);

    PWS_dialog.cryptostick = cryptostick;

    PWS_dialog.exec();
/**/
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
        msgBox.setText("Can't send password chars via HID");
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
        cryptostick->getPasswordRetryCount();

        QString password = QInputDialog::getText(this,tr("Enter card admin password"),tr("Admin password: ")+tr("(Tries left: ")+QString::number(cryptostick->passwordRetryCount)+")", QLineEdit::Password,"", &ok);

        if (ok){

            uint8_t tempPassword[25];

            if ((0 == strcmp (password.toLatin1().data(), "123456")) || (0 == strcmp (password.toLatin1().data(), "12345678")))
            {
                QMessageBox   msgBox;
                msgBox.setText("Warning: Default PIN is used.\nPlease change the PIN");
                msgBox.exec();
            }

            for (int i=0;i<25;i++)
                tempPassword[i]=qrand()&0xFF;
            cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(),tempPassword);
            if (cryptostick->validPassword){
                lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
            }
            password.clear();
        }
    }

// Start the config dialog
    if (cryptostick->validPassword){
        cryptostick->setTime(TOTP_SET_TIME);
   }
    else if (ok){
        QMessageBox msgBox;
         msgBox.setText("Invalid password!");
         msgBox.exec();
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

            QString password = QInputDialog::getText(this, tr("Enter card user password"),tr("User password: ")+tr("(Tries left: ")+QString::number(cryptostick->userPasswordRetryCount)+")", QLineEdit::Password,"", &ok);

            if (TRUE == ok)
            {
                uint8_t tempPassword[25];

                if ((0 == strcmp (password.toLatin1().data(), "123456")) || (0 == strcmp (password.toLatin1().data(), "12345678")))
                {
                    QMessageBox   msgBox;
                    msgBox.setText("Warning: Default PIN is used.\nPlease change the PIN");
                    msgBox.exec();
                }

                for (int i=0;i<25;i++)
                    tempPassword[i]=qrand()&0xFF;

                cryptostick->userAuthenticate((uint8_t *)password.toLatin1().data(),tempPassword);

                if (cryptostick->validUserPassword){
                    lastUserAuthenticateTime = QDateTime::currentDateTime().toTime_t();
                }
                password.clear();
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

     if(ret == -2){
         QMessageBox msgBox;
         msgBox.setWindowTitle("Time is out-of-sync");
         msgBox.setText("WARNING!\n\nThe time of your computer and Crypto Stick are out of sync.\nYour computer may be configured with a wrong time or\nyour Crypto Stick may have been attacked. If an attacker or\nmalware could have used your Crypto Stick you should reset the secrets of your configured One Time Passwords. If your computer's time is wrong, please configure it correctly and reset the time of your Crypto Stick.\n\nReset Crypto Stick's time?");
         msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
         msgBox.setDefaultButton(QMessageBox::No);
         ret = msgBox.exec();

         switch (ret) {
           case QMessageBox::Yes:
                resetTime();
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                Sleep::msleep(1000);
                QApplication::restoreOverrideCursor();
                generateAllConfigs();

                msgBox.setText("Time reset!");
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.exec();

               break;
           case QMessageBox::No:
               return 1;
               break;
           default:
               // should never be reached
               break;
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
         QMessageBox msgBox;
          msgBox.setText("Invalid password!");
          msgBox.exec();
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

