/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
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

#include <QDateTime>
#include <QDesktopWidget>
#include <QMenu>
#include <QTimer>
#include <QGraphicsScene>

#include "nitrokey-applet.h"

#include "stick20-response-task.h"
#include "stick20responsedialog.h"
#include "ui_stick20responsedialog.h"

class OwnSleep : public QThread {
public:
  static void usleep(unsigned long usecs) { QThread::usleep(usecs); }
  static void msleep(unsigned long msecs) { QThread::msleep(msecs); }
  static void sleep(unsigned long secs) { QThread::sleep(secs); }
};

Stick20ResponseDialog::Stick20ResponseDialog(QWidget *parent,
                                             Stick20ResponseTask *Stick20TaskPointer)
    : QDialog(parent), ui(new Ui::Stick20ResponseDialog) {
  ui->setupUi(this);

  QGraphicsScene Scene;

  QSize SceneSize;

  QMovie *ProgressMovie = new QMovie(":/images/progressWheel2.gif");

  ui->LabelProgressWheel->setAttribute(Qt::WA_TranslucentBackground, true);

  // Center dialog to the screen
  this->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, this->size(),
                                        QApplication::desktop()->availableGeometry()));

  Stick20Task = Stick20TaskPointer;

  if (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS != Stick20Task->ActiveCommand) {
    this->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    ui->HeaderText->hide();
  } else {
    this->setWindowFlags(Qt::Window);
  }

  if (FALSE == DebugingActive) // Resize the dialog when debugging is
                               // inactive
  {
    ui->OutputText->hide();
    ui->OutputText->setText("");
    // Start progress wheel
    if (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS != Stick20Task->ActiveCommand) {
      ui->LabelProgressWheel->show();

      SceneSize.setHeight(60);
      SceneSize.setWidth(60);

      ProgressMovie->setScaledSize(SceneSize);
      ui->LabelProgressWheel->setMovie(ProgressMovie);
      ProgressMovie->start();

      ui->progressBar->hide();
    } else {
      ui->label->hide();
    }
  } else {
    ui->HeaderText->show();
    ui->LabelProgressWheel->hide();
    ui->progressBar->hide();
  }

  pollStick20Timer = new QTimer(this);

  // Start timer for polling stick response
  connect(pollStick20Timer, SIGNAL(timeout()), this, SLOT(checkStick20StatusDialog()));
  pollStick20Timer->start(100);

  this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

Stick20ResponseDialog::~Stick20ResponseDialog() {
  delete ui;

  // Kill timer
  pollStick20Timer->stop();

  delete pollStick20Timer;
}

#include "libnitrokey/include/NitrokeyManager.h"
//void Stick20ResponseDialog::showStick20Configuration(int Status) {
//  auto m = nitrokey::NitrokeyManager::instance();
//  auto status = m->get_status_storage();

//  QString OutputText;
//
//  Status = 0;
//  if (0 == Status) {
//    OutputText.append(QString("Firmware version      "));
//    OutputText.append(
//        QString("%1").arg(QString::number(HID_Stick20Configuration_st.VersionInfo_au8[0])));
//    OutputText.append(QString("."));
//    OutputText.append(
//        QString("%1").arg(QString::number(HID_Stick20Configuration_st.VersionInfo_au8[1])));
//    OutputText.append(QString("\n"));
//
//    if (TRUE == HID_Stick20Configuration_st.FirmwareLocked_u8) {
//      OutputText.append(QString("    *** Firmware is locked *** ")).append("\n");
//    }
//
//    if (READ_WRITE_ACTIVE == HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8) {
//      OutputText.append(QString("Uncrypted volume     READ/WRITE mode ")).append("\n");
//    } else {
//      OutputText.append(QString("Uncrypted volume     READ ONLY mode ")).append("\n");
//    }
//
//    if (0 !=
//        (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_CRYPTED_VOLUME_BIT_PLACE))) {
//      OutputText.append(QString("Crypted volume        active")).append("\n");
//    } else {
//      OutputText.append(QString("Crypted volume        not active")).append("\n");
//    }
//
//    if (0 !=
//        (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_HIDDEN_VOLUME_BIT_PLACE))) {
//      OutputText.append(QString("Hidden volume         active")).append("\n");
//    }
//
//    if (0 != (HID_Stick20Configuration_st.NewSDCardFound_u8 & 0x01)) {
//      OutputText.append(QString("*** New SD card found - Change Counter "));
//      OutputText
//          .append(QString("%1").arg(
//              QString::number(HID_Stick20Configuration_st.NewSDCardFound_u8 >> 1)))
//          .append("\n");
//    } else {
//      OutputText.append(QString("SD card              Change Counter "));
//      OutputText
//          .append(QString("%1").arg(
//              QString::number(HID_Stick20Configuration_st.NewSDCardFound_u8 >> 1)))
//          .append("\n");
//    }
//
//    if (0 == (HID_Stick20Configuration_st.SDFillWithRandomChars_u8 & 0x01)) {
//      OutputText.append(QString("*** Not initialized with random data - Fill Counter "));
//      OutputText
//          .append(QString("%1").arg(
//              QString::number(HID_Stick20Configuration_st.SDFillWithRandomChars_u8 >> 1)))
//          .append("\n");
//    } else {
//      OutputText.append(QString("Filled with random    Fill Counter "));
//      OutputText
//          .append(QString("%1").arg(
//              QString::number(HID_Stick20Configuration_st.SDFillWithRandomChars_u8 >> 1)))
//          .append("\n");
//    }
//
//    OutputText.append(QString("Smartcard ID "));
//    OutputText
//        .append(
//            QString("%1").arg(QString::number(HID_Stick20Configuration_st.ActiveSmartCardID_u32)))
//        .append("\n");
//
//    OutputText.append(QString("SD ID "));
//    OutputText
//        .append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.ActiveSD_CardID_u32)))
//        .append("\n");
//
//    OutputText.append(QString("Admin PIN retry counter "));
//    OutputText
//        .append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.AdminPwRetryCount)))
//        .append("\n");
//
//    OutputText.append(QString("User PIN retry counter  "));
//    OutputText
//        .append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.UserPwRetryCount)))
//        .append("\n");
//  }
//  /* else { OutputText.append(QString("Can't read HID interface\n")); } */
//  ui->OutputText->setText(OutputText);
//  ui->OutputText->show();
//  DebugAppendTextGui(OutputText.toLatin1().data());
//}

//void Stick20ResponseDialog::checkStick20StatusDebug(Response *stick20Response, int Status) {
//  QString OutputText;
//
//  OutputText.append(QByteArray::number(Stick20Task->Counter_u32, 10)).append(" Calls\n");
//
//  if (0 == Status) {
//    OutputText.append(QString("CommandCounter   "))
//        .append(QByteArray::number(stick20Response->HID_Stick20Status_st.CommandCounter_u8))
//        .append("\n");
//    OutputText.append(QString("LastCommand      "))
//        .append(QByteArray::number(stick20Response->HID_Stick20Status_st.LastCommand_u8))
//        .append("\n");
//    OutputText.append(QString("Status           "))
//        .append(QByteArray::number(stick20Response->HID_Stick20Status_st.Status_u8))
//        .append("\n");
//    OutputText.append(QString("ProgressBarValue "))
//        .append(QByteArray::number(stick20Response->HID_Stick20Status_st.ProgressBarValue_u8))
//        .append("\n");
//  } else {
//    OutputText.append(QString("Can't read HID interface\n"));
//  }
//
//  ui->OutputText->setText(OutputText);
//  ui->OutputText->show();
//}

void Stick20ResponseDialog::checkStick20StatusDialog(void) {
//  QString OutputText;
//
//  if (STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS == Stick20Task->ActiveCommand) {
//    ui->HeaderText->show();
//  }
//
//  Stick20Task->checkStick20Status();
//
//  if (TRUE == DebugingActive) {
//    checkStick20StatusDebug(Stick20Task->stick20Response, Stick20Task->retStick20Respone);
//  }
//
//  if (0 == Stick20Task->retStick20Respone) {
//    switch (Stick20Task->stick20Response->HID_Stick20Status_st.LastCommand_u8) {
//    case STICK20_CMD_ENABLE_CRYPTED_PARI:
//      OutputText.append(QString("Enabling encrypted volume"));
//      break;
//    case STICK20_CMD_DISABLE_CRYPTED_PARI:
//      OutputText.append(QString("Disabling encrypted volume"));
//      break;
//    case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI:
//      OutputText.append(QString("Enabling encrypted volume"));
//      break;
//    case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI:
//      OutputText.append(QString("Disabling encrypted volume"));
//      break;
//    case STICK20_CMD_SEND_NEW_PASSWORD:
//      OutputText.append(QString("Changing PIN"));
//      break;
//    case STICK20_CMD_ENABLE_FIRMWARE_UPDATE:
//      OutputText.append(QString("Enabling firmeware update"));
//      break;
//    case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE:
//      OutputText.append(QString("Exporting firmware to file"));
//      break;
//    case STICK20_CMD_GENERATE_NEW_KEYS:
//      OutputText.append(QString("Generating new keys"));
//      break;
//    case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS:
//      OutputText.append(QString("Initializing storage with random data"));
//      break;
//    case STICK20_CMD_GET_DEVICE_STATUS:
//      OutputText.append(QString("Getting device configuration"));
//      break;
//    case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
//      OutputText.append(QString("Enabling read-only configuration for the unencrypted volume"));
//      break;
//    case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
//      OutputText.append(QString("Enabling read-write configuration for the unencrypted volume"));
//      break;
//    case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND:
//      OutputText.append(QString("Disabling 'initialize storage with random data' warning"));
//      break;
//    case STICK20_CMD_PRODUCTION_TEST:
//      OutputText.append(QString("Production test"));
//      break;
//    case STICK20_CMD_SEND_STARTUP:
//      OutputText.append(QString("Startup infos"));
//      break;
//    case STICK20_CMD_SEND_LOCK_STICK_HARDWARE:
//      OutputText.append(QString("Locking"));
//      break;
//
//    default:
//      break;
//    }
//
//    OutputText.append(QString(" ("));
//
//    switch (Stick20Task->stick20Response->HID_Stick20Status_st.Status_u8) {
//    case OUTPUT_CMD_STICK20_STATUS_IDLE:
//      OutputText.append(QString("IDLE"));
//      break;
//    case OUTPUT_CMD_STICK20_STATUS_OK:
//      OutputText.append(QString("OK"));
//      pollStick20Timer->stop();
//      done(TRUE);
//      break;
//    case OUTPUT_CMD_STICK20_STATUS_BUSY:
//      OutputText.append(QString("BUSY"));
//      break;
//    case OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD:
//      OutputText.append(QString("WRONG PASSWORD"));
//      pollStick20Timer->stop();
//      done(TRUE);
//      break;
//    case OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR:
//      OutputText.append(QString("BUSY"));
//      ui->progressBar->show();
//      ui->LabelProgressWheel->hide();
//      ui->progressBar->setValue(
//          Stick20Task->stick20Response->HID_Stick20Status_st.ProgressBarValue_u8);
//      break;
//    case OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY:
//      OutputText.append(QString("PASSWORD MATRIX READY"));
//      pollStick20Timer->stop();
//      done(TRUE);
//      break;
//    case OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK:
//      OutputText.append(QString("NO USER PASSWORD UNLOCK"));
//      pollStick20Timer->stop();
//      done(TRUE);
//      break;
//    case OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR:
//      OutputText.append(QString("SMARTCARD ERROR"));
//      pollStick20Timer->stop();
//      done(TRUE);
//      break;
//    case OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE:
//      OutputText.append(QString("SECURITY BIT ACTIVE"));
//      pollStick20Timer->stop();
//      done(TRUE);
//      break;
//    default:
//      break;
//    }
//    OutputText.append(QString(")"));
//    ui->HeaderText->setText(OutputText);
//    ui->label->setText(OutputText);
//  }
}
