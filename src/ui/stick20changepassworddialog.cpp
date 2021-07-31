/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
 * Copyright (c) 2013-2018 Nitrokey UG
 *
 * This file is part of Nitrokey App.
 *
 * Nitrokey App is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey App is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include "stick20changepassworddialog.h"
#include "nitrokey-applet.h"
#include "stick20responsedialog.h"
#include "ui_stick20changepassworddialog.h"
#include "libada.h"
#include <libnitrokey/NitrokeyManager.h>
#include "src/core/ThreadWorker.h"
#include <QStyle>
using nm = nitrokey::NitrokeyManager;

DialogChangePassword::DialogChangePassword(QWidget *parent, PasswordKind _kind):
    QDialog(parent),
    ui(new Ui::DialogChangePassword),
    kind(_kind)
{
  ui->setupUi(this);

  ui->lineEdit_OldPW->setEchoMode(QLineEdit::Password);
  ui->lineEdit_OldPW->setMaxLength(STICK20_PASSOWRD_LEN); // TODO change define to constant
  ui->lineEdit_NewPW_1->setEchoMode(QLineEdit::Password);
  ui->lineEdit_NewPW_1->setMaxLength(STICK20_PASSOWRD_LEN); // TODO change to
                                                            // UI_PASSWORD_LEN
                                                            // this and other
                                                            // occurences
  ui->lineEdit_NewPW_2->setEchoMode(QLineEdit::Password);
  ui->lineEdit_NewPW_2->setMaxLength(STICK20_PASSOWRD_LEN);

  fixWindowGeometry();
  ui->lineEdit_OldPW->setFocus();
  setModal(true);
  connect(parent, SIGNAL(LongOperationStart()), this, SLOT(reject()));
}

DialogChangePassword::~DialogChangePassword() { delete ui; }

void DialogChangePassword::fixWindowGeometry() {
  resize(0, 0);
  adjustSize();
  updateGeometry();
  moveWindowToCenter();
}

void DialogChangePassword::UpdatePasswordRetry() {
  QString noTrialsLeft;
  // update password retry values

  switch (kind) {
    case PasswordKind::USER:
    noTrialsLeft = tr("Unfortunately you have no more trials left. Please use 'Reset User PIN' "
                      "option from menu to reset password");
    break;
    case PasswordKind::ADMIN:
    case PasswordKind::RESET_USER:
    noTrialsLeft = tr("Unfortunately you have no more trials left. Please check instruction how to "
                      "reset Admin password.");
    break;
    case PasswordKind::UPDATE:
    ui->retryCount->hide();
    ui->retryCountLabel->hide();
    break;
  }

  PasswordKind k = kind;
  ui->retryCount->setText("...");
  new ThreadWorker(
    [k]() -> Data {
      Data data;
      switch (k) {
        case PasswordKind::USER:
          data["counter"] = libada::i()->getUserPasswordRetryCount();
        break;
        case PasswordKind::ADMIN:
        case PasswordKind::RESET_USER:
          data["counter"] = libada::i()->getAdminPasswordRetryCount();
        break;
        case PasswordKind::UPDATE:
          data["counter"] = 99;
          break;
      }
      return data;
    },
    [this, noTrialsLeft](Data data){
      int retryCount_local = data["counter"].toInt();
      ui->retryCount->setText(QString::number(retryCount_local));
      ui->retryCount->repaint();
      fixWindowGeometry();
      if (retryCount_local == 0) {
        QString cssRed = "QLabel {color: red; font-weight: bold}";
        ui->retryCount->setStyleSheet(cssRed);
        ui->retryCountLabel->setStyleSheet(cssRed);
        csApplet()->warningBox(noTrialsLeft);
        done(true);
        emit UserPinLocked();
      }
    }, this);
}

void DialogChangePassword::UI_deviceNotInitialized() const { csApplet()->warningBox(tr("Device is not yet initialized. Please try again later.")); }


int DialogChangePassword::exec() {
  if (!libada::i()->isDeviceInitialized()) {
    UI_deviceNotInitialized();
    done(Rejected);
    return QDialog::Rejected;
  }
  return QDialog::exec();
}

#include <QDesktopWidget>
void DialogChangePassword::InitData(void) {
  moveWindowToCenter();

  this->UpdatePasswordRetry();

  minimumPasswordLength = minimumPasswordLengthUser;
  switch (kind) {
    case PasswordKind::USER:
      this->setWindowTitle(tr("Set User PIN"));
      ui->label_2->setText(tr("Current User PIN:"));
      ui->label_3->setText(tr("New PIN:"));
      ui->label_4->setText(tr("Confirm New PIN:"));
      minimumPasswordLength = minimumPasswordLengthUser;
      break;
    case PasswordKind::ADMIN:
      this->setWindowTitle(tr("Set Admin PIN"));
      ui->label_2->setText(tr("Current Admin PIN:"));
      ui->label_3->setText(tr("New PIN:"));
      ui->label_4->setText(tr("Confirm New PIN:"));
      minimumPasswordLength = minimumPasswordLengthAdmin;
      break;
    case PasswordKind::RESET_USER:
      this->setWindowTitle(tr("Reset User PIN"));
      ui->label_2->setText(tr("Current Admin PIN:"));
      ui->label_3->setText(tr("New User PIN:"));
      ui->label_4->setText(tr("Confirm New User PIN:"));
      minimumPasswordLength = minimumPasswordLengthUser;
      break;
    case PasswordKind::UPDATE:
      this->setWindowTitle(tr("Change Firmware Password"));
      ui->label_2->setText(tr("Current Firmware Password:"));
      ui->label_3->setText(tr("New Password:"));
      ui->label_4->setText(tr("Confirm New Password:"));
      ui->checkBox->setText(tr("Show password"));
      minimumPasswordLength = minimumPasswordLengthFirmware;
      break;
  }

  ui->lineEdit_OldPW->setAccessibleName(ui->label_2->text());
  ui->lineEdit_NewPW_1->setAccessibleName(ui->label_3->text());
  ui->lineEdit_NewPW_2->setAccessibleName(ui->label_4->text());

  // replace %1 and %2 from text with proper values
  //(min and max of password length)
  QString text = ui->label_additional_information->text();

  text = text.arg(minimumPasswordLength).arg(STICK20_PASSOWRD_LEN);

  if (kind == PasswordKind::UPDATE) {
    text = "<i> " + tr("The Firmware password doesn’t have a retry counter, and therefore doesn’t prevent against password guessing attacks. "
                       "A secure and complex password should be created with the use of: "
                       "lower and upper case letters, numbers and special characters; with a length between %2 and %3 characters."
                     "<br/>Default firmware password is: '%1'.").arg(defaultFirmwarePassword).arg(minimumPasswordLengthFirmware).arg(maximumPasswordLengthFirmware)
            + "</i>";
    text += QString("<p><b>") + tr("WARNING! If you lose your Firmware password, Nitrokey can’t be updated or reset!") + QString("</b></p>");

  }
  ui->label_additional_information->setText(text);
  fixWindowGeometry();
}

void DialogChangePassword::moveWindowToCenter() {// center the password window
  QDesktopWidget *desktop = QApplication::desktop();
  setGeometry(
      QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), desktop->availableGeometry()));
}

#include <QDebug>

void DialogChangePassword::_changePassword(void) {
  QByteArray PasswordString, PasswordStringNew;
  PasswordString = ui->lineEdit_OldPW->text().toLatin1();
  PasswordStringNew = ui->lineEdit_NewPW_1->text().toLatin1();

  PasswordKind k = kind;
  new ThreadWorker(
      [k, PasswordString, PasswordStringNew]() -> Data {
        Data data;
        data["update_feature"] = true;
        try {
          switch (k) {
          case PasswordKind::USER:
            nm::instance()->change_user_PIN(PasswordString.constData(),
                                            PasswordStringNew.constData());
            break;
          case PasswordKind::ADMIN:
            nm::instance()->change_admin_PIN(PasswordString.constData(),
                                             PasswordStringNew.constData());
            break;
          case PasswordKind::UPDATE:
            switch (nm::instance()->get_connected_device_model()) {
            case nitrokey::device::DeviceModel::STORAGE:
              nm::instance()->change_update_password(PasswordString.constData(),
                                                     PasswordStringNew.constData());
              break;
            case nitrokey::device::DeviceModel::PRO:
              if (nm::instance()->get_major_firmware_version() > 0
                  || ( nm::instance()->get_major_firmware_version() == 0
                  && nm::instance()->get_minor_firmware_version() >= 11) ){
                nm::instance()->change_firmware_update_password_pro(PasswordString.constData(),
                                                       PasswordStringNew.constData());
              } else {
                // TODO inform this device is not capable of firmware updates
                data["result"] = false;
                data["update_feature"] = false;
              }
              break;
            default:
              qDebug() << "Unknown device";
              break;
            }
            break;
          case PasswordKind::RESET_USER:
            nm::instance()->unlock_user_password(PasswordString.constData(),
                                                 PasswordStringNew.constData());
            break;
          }
          data["result"] = true;
        }
        catch (CommandFailedException &e){
          if(!e.reason_wrong_password()){
            data["err"] = e.last_command_status;
//            emit this->error(e.last_command_status);
          }
          data["result"] = false;
        }
        return data;
      },
      [this](Data data){
        const bool result = data["result"].toBool();
        const bool update_feature = data["update_feature"].toBool();
        if (!update_feature) {
          csApplet()->warningBox(tr("This device does not have firmware update feature."));
          done(true);
        } else if (!result){
          csApplet()->warningBox(tr("Current password is not correct. Please retry."));
          this->UpdatePasswordRetry();
          this->clearFields();
        }
        else{
          csApplet()->messageBox(tr("New password is set"));
          done(true);
        }
      }, this);
}

void DialogChangePassword::accept() {
  // Check the length of the old password
  QString OutputText;
  if (minimumPasswordLength > ui->lineEdit_OldPW->text().toLatin1().length()) {
    clearFields();
    OutputText = tr("The minimum length of the old password is ") +
                 QString("%1").arg(minimumPasswordLength) + tr(" chars");

    csApplet()->warningBox(OutputText);
    return;
  }

  // Check for correct new password entries
  if (ui->lineEdit_NewPW_1->text() != ui->lineEdit_NewPW_2->text()) {
    clearFields();
    csApplet()->warningBox(tr("The new password entries are not the same"));
    return;
  }

  // Check the new length of password - max
  // obsolete since input field max length is set
  if (STICK20_PASSOWRD_LEN < ui->lineEdit_NewPW_1->text().toLatin1().length()) {
    clearFields();
    OutputText = tr("The maximum length of a password is ") +
                 QString("%1").arg(STICK20_PASSOWRD_LEN) + tr(" chars");

    csApplet()->warningBox(OutputText);
    return;
  }

  // Check the new length of password - min
  if (minimumPasswordLength > ui->lineEdit_NewPW_1->text().toLatin1().length()) {
    clearFields();
    OutputText = tr("The minimum length of a password is ") +
                 QString("%1").arg(minimumPasswordLength) + tr(" chars");

    csApplet()->warningBox(OutputText);
    return;
  }

  _changePassword();
}

void DialogChangePassword::on_checkBox_clicked(bool checked) {
  auto mode = checked ? QLineEdit::Normal : QLineEdit::Password;
  ui->lineEdit_OldPW->setEchoMode(mode);
  ui->lineEdit_NewPW_1->setEchoMode(mode);
  ui->lineEdit_NewPW_2->setEchoMode(mode);
}

void DialogChangePassword::clearFields() {
  ui->lineEdit_OldPW->clear();
  ui->lineEdit_NewPW_1->clear();
  ui->lineEdit_NewPW_2->clear();

  ui->lineEdit_OldPW->setFocus();
}
