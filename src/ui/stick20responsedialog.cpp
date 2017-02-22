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
#include <QTimer>
#include <QGraphicsScene>

#include "nitrokey-applet.h"

//#include "stick20-response-task.h"
#include "stick20responsedialog.h"
#include "ui_stick20responsedialog.h"
#include "libada.h"

Stick20ResponseDialog::Stick20ResponseDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::Stick20ResponseDialog) {
  ui->setupUi(this);

  ui->LabelProgressWheel->setAttribute(Qt::WA_TranslucentBackground, true);

  center_window();

//  bool should_show_progress_wheel = false; //STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS != Stick20Task->ActiveCommand;
//  bool no_debug = true; // todo FALSE == DebugingActive;

//  set_window_type(Type::wheel, false);

  //  pollStick20Timer = new QTimer(this);

  // Start timer for polling stick response
//  connect(pollStick20Timer, SIGNAL(timeout()), this, SLOT(checkStick20StatusDialog()));
//  pollStick20Timer->start(100);

  this->layout()->setSizeConstraint(QLayout::SetFixedSize);
  ProgressMovie = std::make_shared<QMovie>(":/images/progressWheel2.gif");
}

void Stick20ResponseDialog::center_window() {
  setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(),
                                  QApplication::desktop()->availableGeometry()));
}

void Stick20ResponseDialog::set_window_type(Type type, bool no_debug, QString text) {
  ui->HeaderText->setText(text);
  ui->label->setText(text);

  if (current_type == type) return;

  if (type == Type::wheel) {
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    ui->HeaderText->hide();
  } else if (type == Type::progress_bar){
    setWindowFlags(Qt::Window);
  }

  if (no_debug) // Resize the dialog when debugging is inactive
  {
    ui->OutputText->hide();
    ui->OutputText->setText("");

    ui->LabelProgressWheel->setVisible(type == Type::wheel && no_debug);
    ui->label->setVisible(type == Type::wheel);
    ui->progressBar->setVisible(type == Type::progress_bar && no_debug);

    // Start progress wheel
    if (type == Type::wheel) {
      QSize SceneSize;
      SceneSize.setHeight(60);
      SceneSize.setWidth(60);
      ProgressMovie->setScaledSize(SceneSize);
      ui->LabelProgressWheel->setMovie(ProgressMovie.get());
      ProgressMovie->start();
    }
  } else {
    ui->HeaderText->show();
    ui->progressBar->hide();
  }
  current_type = type;
  this->show();
}

Stick20ResponseDialog::~Stick20ResponseDialog() {
  delete ui;

//   Kill timer
//  pollStick20Timer->stop();

//  delete pollStick20Timer;
}

#include "libnitrokey/include/NitrokeyManager.h"

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

void Stick20ResponseDialog::updateOperationInProgressBar(int p) {
  init_long_operation();
  ui->progressBar->setValue(p);
  if(p==100){
    QTimer::singleShot(3*1000, [this](){
      this->hide();
    });
  }
}

void Stick20ResponseDialog::init_long_operation() {
  QString description_string;
  description_string.append("Initializing storage with random data");
  description_string.append(" (BUSY)");

  set_window_type(Type::progress_bar, true, description_string);
}
