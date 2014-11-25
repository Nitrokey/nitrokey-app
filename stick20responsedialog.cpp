/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
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

#include <QTimer>
#include <QMenu>
#include <QtGui>
#include <QDateTime>

#include "device.h"
#include "response.h"
#include "cryptostick-applet.h"

#include "stick20responsedialog.h"
#include "ui_stick20responsedialog.h"

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

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

  Stick20ResponseDialog

  Constructor Stick20ResponseDialog

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

Stick20ResponseDialog::Stick20ResponseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Stick20ResponseDialog)
{
    int Value;

    ui->setupUi(this);

    QGraphicsScene Scene;
    QSize SceneSize;
    QMovie *ProgressMovie = new QMovie(":/images/ProgressWheel.GIF");

    if (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS != Stick20Task->ActiveCommand)
    {
        ui->HeaderText->hide ();
    }

    if (FALSE == DebugingActive)             // Resize the dialog when debugging is inactiv
    {
// Start progress wheel
        if (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS != Stick20Task->ActiveCommand)
        {
            ui->LabelProgressWheel->show();

            SceneSize.setHeight(60);
            SceneSize.setWidth(60);

            ProgressMovie->setScaledSize(SceneSize);
            ui->LabelProgressWheel->setMovie(ProgressMovie);
            ProgressMovie->start();

            ui->OutputText->setText("");
            ui->progressBar->hide();
        }

// Resize window
        ui->OutputText->hide ();
        Value = ui->OutputText->height();

        QSize dialogSize = this->size();
        dialogSize.setHeight (dialogSize.height() - Value);
        this->resize(dialogSize);
    }
    else
    {
        ui->HeaderText->show();
        ui->LabelProgressWheel->hide();
        ui->progressBar->hide();
    }

    pollStick20Timer = new QTimer(this);

    // Start timer for polling stick response
    connect(pollStick20Timer, SIGNAL(timeout()), this, SLOT(checkStick20StatusDialog()));
    pollStick20Timer->start(100);

    ui->pushButton->hide ();
}

/*******************************************************************************

  ShowStick20Configuration

  Changes
  Date      Author        Info
  05.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void Stick20ResponseDialog::showStick20Configuration (int Status)
{
    QString OutputText;


    Status = 0;
    if (0 == Status)
    {
        OutputText.append(QString("Firmware version      "));
        OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.VersionInfo_au8[0])));
        OutputText.append(QString("."));
        OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.VersionInfo_au8[1])));
        OutputText.append(QString("\n"));

        if (TRUE == HID_Stick20Configuration_st.FirmwareLocked_u8)
        {
            OutputText.append(QString("    *** Firmware is locked *** ")).append("\n");
        }

        if (READ_WRITE_ACTIVE == HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8)
        {
            OutputText.append(QString("Uncrypted volume     READ/WRITE mode ")).append("\n");
        }
        else
        {
            OutputText.append(QString("Uncrypted volume     READ ONLY mode ")).append("\n");
        }

        if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_CRYPTED_VOLUME_BIT_PLACE)))
        {
            OutputText.append(QString("Crypted volume        active")).append("\n");
        }
        else
        {
            OutputText.append(QString("Crypted volume        not active")).append("\n");
        }

        if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_HIDDEN_VOLUME_BIT_PLACE)))
        {
            OutputText.append(QString("Hidden volume         active")).append("\n");
        }

        if (0 != (HID_Stick20Configuration_st.NewSDCardFound_u8 & 0x01))
        {
            OutputText.append(QString("*** New SD card found - Change Counter "));
            OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.NewSDCardFound_u8 >> 1))).append("\n");
        }
        else
        {
            OutputText.append(QString("SD card              Change Counter "));
            OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.NewSDCardFound_u8 >> 1))).append("\n");
        }

        if (0 == (HID_Stick20Configuration_st.SDFillWithRandomChars_u8 & 0x01))
        {
            OutputText.append(QString("*** Not initialized with random data - Fill Counter "));
            OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.SDFillWithRandomChars_u8 >> 1))).append("\n");
        }
        else
        {
            OutputText.append(QString("Filled with random    Fill Counter "));
            OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.SDFillWithRandomChars_u8 >> 1))).append("\n");
        }

        OutputText.append(QString("Smartcard ID "));
        OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.ActiveSmartCardID_u32))).append("\n");

        OutputText.append(QString("SD ID "));
        OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.ActiveSD_CardID_u32))).append("\n");

        OutputText.append(QString("Admin PIN retry counter "));
        OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.AdminPwRetryCount))).append("\n");

        OutputText.append(QString("User PIN retry counter  "));
        OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.UserPwRetryCount))).append("\n");

    }
    else
    {
        OutputText.append(QString("Can't read HID interface\n"));
    }

    ui->OutputText->setText(OutputText);

    DebugAppendText (OutputText.toLatin1().data());

}

/*******************************************************************************

  checkStick20StatusDebug

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Stick20ResponseDialog::checkStick20StatusDebug(Response *stick20Response,int Status)
{
    QString OutputText;

    OutputText.append(QByteArray::number(Stick20Task->Counter_u32,10)).append(" Calls\n");

    if (0 == Status)
    {
        OutputText.append(QString("CommandCounter   ")).append(QByteArray::number(stick20Response->HID_Stick20Status_st.CommandCounter_u8)).append("\n");
        OutputText.append(QString("LastCommand      ")).append(QByteArray::number(stick20Response->HID_Stick20Status_st.LastCommand_u8)).append("\n");
        OutputText.append(QString("Status           ")).append(QByteArray::number(stick20Response->HID_Stick20Status_st.Status_u8)).append("\n");
        OutputText.append(QString("ProgressBarValue ")).append(QByteArray::number(stick20Response->HID_Stick20Status_st.ProgressBarValue_u8)).append("\n");
    }
    else
    {
        OutputText.append(QString("Can't read HID interface\n"));
    }

    ui->OutputText->setText(OutputText);
}

/*******************************************************************************

  checkStick20StatusDialog

  Changes
  Date      Author        Info
  10.10.14  RB            Use the function checkStick20Status of Stick20ResponseTask

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void Stick20ResponseDialog::checkStick20StatusDialog()
{
    QString OutputText;

    if (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS == Stick20Task->ActiveCommand)
    {
        ui->HeaderText->show ();
    }

    Stick20Task->checkStick20Status();

    if (TRUE == DebugingActive)
    {
        checkStick20StatusDebug (Stick20Task->stick20Response,Stick20Task->retStick20Respone);
    }

    if (0 == Stick20Task->retStick20Respone)
    {
        switch (Stick20Task->stick20Response->HID_Stick20Status_st.LastCommand_u8)
        {
            case STICK20_CMD_ENABLE_CRYPTED_PARI            :
                OutputText.append (QString("Enable encrypted volume"));
                break;
            case STICK20_CMD_DISABLE_CRYPTED_PARI           :
                OutputText.append (QString("Disable encrypted volume"));
                break;
            case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI     :
                OutputText.append (QString("Enable encrypted volume"));
                break;
            case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI    :
                OutputText.append (QString("Disable encrypted volume"));
                break;
            case STICK20_CMD_SEND_NEW_PASSWORD    :
                OutputText.append (QString("Change PIN"));
                break;
            case STICK20_CMD_ENABLE_FIRMWARE_UPDATE         :
                OutputText.append (QString("Enable firmeware update"));
                break;
            case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE        :
                OutputText.append (QString("Export firmware to file"));
                break;
            case STICK20_CMD_GENERATE_NEW_KEYS              :
                OutputText.append (QString("Generate new keys"));
                break;
            case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS :
                OutputText.append (QString("Initialize storage with random data"));
                break;
            case STICK20_CMD_GET_DEVICE_STATUS              :
                OutputText.append (QString("Get device configuration"));
                break;
            case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN  :
                OutputText.append (QString("Enable readonly for unencrypted volume"));
                break;
            case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
                OutputText.append (QString("Enable readwrite for unencrypted volume"));
                break;
            case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND :
                OutputText.append (QString("Disable 'initialize storage with random data' warning"));
                break;
            case STICK20_CMD_PRODUCTION_TEST :
                OutputText.append (QString("Production test"));
                break;
            case STICK20_CMD_SEND_STARTUP :
                OutputText.append (QString("Startup infos"));
                break;
            case STICK20_CMD_SEND_LOCK_STICK_HARDWARE :
                OutputText.append (QString("Lock stick hardware"));
                break;

            default :
                break;
        }

        OutputText.append (QString(" - "));

        switch (Stick20Task->stick20Response->HID_Stick20Status_st.Status_u8)
        {
            case OUTPUT_CMD_STICK20_STATUS_IDLE             :
                OutputText.append (QString("IDLE"));
                break;
            case OUTPUT_CMD_STICK20_STATUS_OK               :
                OutputText.append (QString("OK"));
                pollStick20Timer->stop();
                done (TRUE);
                break;
            case OUTPUT_CMD_STICK20_STATUS_BUSY             :
                OutputText.append (QString("BUSY"));
                break;
            case OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD   :
                OutputText.append (QString("WRONG PASSWORD"));
                pollStick20Timer->stop();
                done (TRUE);
                break;
            case OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR :
                OutputText.append (QString("BUSY"));
                ui->progressBar->show();
                ui->LabelProgressWheel->hide();
                ui->progressBar->setValue(Stick20Task->stick20Response->HID_Stick20Status_st.ProgressBarValue_u8);
                break;
            case OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY   :
                OutputText.append (QString("PASSWORD MATRIX READY"));
                pollStick20Timer->stop();
                done (TRUE);
                break;
            case OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK   :
                OutputText.append (QString("NO USER PASSWORD UNLOCK"));
                pollStick20Timer->stop();
                done (TRUE);
                break;
            case OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR   :
                OutputText.append (QString("SMARTCARD ERROR"));
                pollStick20Timer->stop();
                done (TRUE);
                break;
            case OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE   :
                OutputText.append (QString("SECURITY BIT ACTIVE"));
                pollStick20Timer->stop();
                done (TRUE);
                break;
            default :
                break;
        }
        ui->HeaderText->setText(OutputText);
   }
}


/*******************************************************************************

  ~Stick20ResponseDialog

  Changes
  Date      Author        Info
  02.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

Stick20ResponseDialog::~Stick20ResponseDialog()
{
    delete ui;

// Kill timer
    pollStick20Timer->stop();

    delete pollStick20Timer;
}

/*******************************************************************************

  on_pushButton_clicked

  Changes
  Date      Author          Info
  16.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void Stick20ResponseDialog::on_pushButton_clicked()
{
    done (FALSE);
}

/*******************************************************************************

  Stick20ResponseDialog

  Constructor Stick20ResponseDialog

  Changes
  Date      Author          Info
  02.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Stick20ResponseTask::Stick20ResponseTask(QWidget *parent,Device *Cryptostick20,QSystemTrayIcon *MainWndTrayIcon)
{
    ActiveCommand             = -1;
    EndFlag                   = FALSE;
    FlagNoStopWhenStatusOK    = FALSE;
    ResultValue               = FALSE;
    Counter_u32               = 0;
    retStick20Respone         = 0;

    Stick20ResponseTaskParent = parent;

    cryptostick               = Cryptostick20;
    trayIcon                  = MainWndTrayIcon;

    stick20Response           = new Response();
}

/*******************************************************************************

  NoStopWhenStatusOK

  Changes
  Date      Author          Info
  02.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void Stick20ResponseTask::NoStopWhenStatusOK()
{
    FlagNoStopWhenStatusOK = TRUE;
}

/*******************************************************************************

  ShowIconMessage

  Changes
  Date      Author          Info
  10.10.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void Stick20ResponseTask::ShowIconMessage(QString IconText)
{
    if (TRUE == trayIcon->supportsMessages ())
    {
        trayIcon->showMessage ("Crypto Stick Utility",IconText);
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText(IconText);
        msgBox.exec();
    }
}


/*******************************************************************************

  checkStick20Status

  Changes
  Date      Author          Info
  02.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/
#define RESPONSE_DIALOG_TIME_TO_SHOW_DIALOG 30 // a 100 ms = 3 sec

void Stick20ResponseTask::checkStick20Status()
{
    QString OutputText;

    Counter_u32++;

    retStick20Respone = stick20Response->getResponse(cryptostick);

    if (0 == retStick20Respone)
    {
        if (-1 == ActiveCommand)
        {
            ActiveCommand = stick20Response->HID_Stick20Status_st.LastCommand_u8;
        }

        switch (stick20Response->HID_Stick20Status_st.Status_u8)
        {
            case OUTPUT_CMD_STICK20_STATUS_IDLE             :
                break;
            case OUTPUT_CMD_STICK20_STATUS_OK               :
                EndFlag = TRUE;
                break;
            case OUTPUT_CMD_STICK20_STATUS_BUSY             :
                break;
            case OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD   :
                switch (ActiveCommand)
                {
                    csApplet->warningBox("Get wrong password");
                }            
                EndFlag = TRUE;
                break;
            case OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR :
                break;
            case OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY   :
                EndFlag = TRUE;
                break;
            case OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK   :
                EndFlag = TRUE;
                break;
            case OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR   :
                EndFlag = TRUE;
                break;
            case OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE   :
                EndFlag = TRUE;
                break;
            default :
                break;
        }

        if (TRUE == FlagNoStopWhenStatusOK)
        {
            switch (stick20Response->HID_Stick20Status_st.Status_u8)
            {
                case OUTPUT_CMD_STICK20_STATUS_OK               :
                    done (TRUE);
                    ResultValue = TRUE;
                    break;
                case OUTPUT_CMD_STICK20_STATUS_IDLE             :
                case OUTPUT_CMD_STICK20_STATUS_BUSY             :
                case OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR :
                case OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY   :
                    // Do nothing, wait for next hid info
                    break;
                case OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD   :
                    done (FALSE);
                    ResultValue = FALSE;
                    switch (ActiveCommand)
                    {
                        case STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED :
                        case STICK20_CMD_SEND_LOCK_STICK_HARDWARE :
                        case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE :
                        case STICK20_CMD_GENERATE_NEW_KEYS :
                        case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS :
                            if (0 < HID_Stick20Configuration_st.AdminPwRetryCount)
                            {
                                HID_Stick20Configuration_st.AdminPwRetryCount--;
                            }
                            break;
                        case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN :
                        case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
                        case STICK20_CMD_ENABLE_CRYPTED_PARI :
                            if (0 < HID_Stick20Configuration_st.UserPwRetryCount)
                            {
                                HID_Stick20Configuration_st.UserPwRetryCount--;
                            }
                            break;
                    }
                    break;
                case OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK :
                    done (FALSE);
                    ResultValue = FALSE;
                    break;
                case OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR  :
                    done (FALSE);
                    ResultValue = FALSE;
                    break;
                case OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE  :
                    done (FALSE);
                    ResultValue = FALSE;
                    break;
            }
        }

        if (OUTPUT_CMD_STICK20_STATUS_OK == stick20Response->HID_Stick20Status_st.Status_u8)
        {
            ResultValue = TRUE;

            switch (ActiveCommand)
            {
                case STICK20_CMD_ENABLE_CRYPTED_PARI            :
                    ShowIconMessage ("Encrypted volume unlocked successfully");
                    HID_Stick20Configuration_st.UserPwRetryCount = 3;
                    break;
                case STICK20_CMD_DISABLE_CRYPTED_PARI           :
                    ShowIconMessage ("Encrypted volume locked successfully");
                    break;
                case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI            :
                    ShowIconMessage ("Hidden volume unlocked successfully");
                    break;
                case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI           :
                    ShowIconMessage ("Hidden volume locked successfully");
                    break;
                case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP           :
                    ShowIconMessage ("Hidden volume setup successfully");
                    break;
                case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN :
                    ShowIconMessage ("Uncrypted volume is in readonly mode");
                    HID_Stick20Configuration_st.UserPwRetryCount = 3;
                    break;
                case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN :
                    ShowIconMessage ("Uncrypted volume is in readwrite mode");
                    HID_Stick20Configuration_st.UserPwRetryCount = 3;
                    break;
                case STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED :
                    ShowIconMessage ("Warning disabled");
                    HID_Stick20Configuration_st.AdminPwRetryCount = 3;
                    break;
                case STICK20_CMD_SEND_LOCK_STICK_HARDWARE :
                    ShowIconMessage ("Firmware is locked");
                    HID_Stick20Configuration_st.AdminPwRetryCount = 3;
                    break;
                case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE :
                    ShowIconMessage ("Firmware exported successfully");
                    HID_Stick20Configuration_st.AdminPwRetryCount = 3;
                    break;
                case STICK20_CMD_GENERATE_NEW_KEYS :
                    ShowIconMessage ("New keys generated successfully");
                    HID_Stick20Configuration_st.AdminPwRetryCount = 3;
                    break;
                case STICK20_CMD_GET_DEVICE_STATUS              :
//                    showStick20Configuration (ret);
                    break;
                case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS :
                    HID_Stick20Configuration_st.AdminPwRetryCount = 3;
                    {
                        csApplet->messageBox("Storage successfully initialized with random data");
                    }
                    done (TRUE);
                    break;
                default :
                    break;
            }
        }

        if (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD == stick20Response->HID_Stick20Status_st.Status_u8)
        {
            switch (ActiveCommand)
            {
                case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI     :
                    {
                        QMessageBox msgBox;
                        msgBox.setText("Can't enable hidden volume");
                        msgBox.exec();
                    }
                    break;
                default :
                    break;
            }
        }

        if (OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK == stick20Response->HID_Stick20Status_st.Status_u8)
        {
            switch (ActiveCommand)
            {
                case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP :
                    {
                        QMessageBox msgBox;
//                        msgBox.setText("To setup the hidden volume, please enable the encrypted volume to enable smartcard access");
                        msgBox.setText("Please enable the encrypted volume first.");
                        msgBox.exec();
                    }
                    break;
                case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI     :
                    {
                        csApplet->messageBox("Encrypted volume was not enabled, please enable the encrypted volume");
                    }
                    break;
                default :
                    break;
            }
        }

        if (OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR == stick20Response->HID_Stick20Status_st.Status_u8)
        {
            switch (ActiveCommand)
            {
                case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI     :
                    {
                        csApplet->warningBox("Smartcard error, please retry the command");
                    }
                    break;
                default :
                    break;
            }
        }

        if (OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE == stick20Response->HID_Stick20Status_st.Status_u8)
        {
            switch (ActiveCommand)
            {
                case STICK20_CMD_ENABLE_FIRMWARE_UPDATE     :
                    {
                        csApplet->messageBox("Security bit of the device is activated.\nFirmware update is not possible.");
                    }
                    break;
                default :
                    break;
            }
        }

    }
}

/*******************************************************************************

  done

  Changes
  Date      Author          Info
  02.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void Stick20ResponseTask::done (int Status)
{
    Status = 0;             // Status not used
    EndFlag = TRUE;
}


/*******************************************************************************

  GetResponse

  Changes
  Date      Author          Info
  02.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void Stick20ResponseTask::GetResponse(void)
{
    int i;

    for (i=0;i<15;i++)
    {
        OwnSleep::msleep(100);
        checkStick20Status ();
        if (TRUE == EndFlag)
        {
            return;
        }
    }

    if (FALSE == EndFlag)
    {
        Stick20ResponseDialog ResponseDialog(Stick20ResponseTaskParent);

        ResponseDialog.Stick20Task = this;
        ResponseDialog.exec();
    }
}

/*******************************************************************************

  ~Stick20ResponseTask

  Changes
  Date      Author          Info
  02.09.14  RB              Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

Stick20ResponseTask::~Stick20ResponseTask()
{
    delete stick20Response;
}



