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

#include "stick20changepassworddialog.h"
#include "device.h" //for STICK20_PASSOWRD_LEN
#include "mcvs-wrapper.h"
#include "nitrokey-applet.h"
#include "stick20responsedialog.h"
#include "ui_stick20changepassworddialog.h"

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

  DialogChangePassword

  Constructor DialogChangePassword

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

DialogChangePassword::DialogChangePassword(QWidget *parent)
    : QDialog(parent), ui(new Ui::DialogChangePassword) {
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

  ui->lineEdit_OldPW->setFocus();
  cryptostick = NULL;
  PasswordKind = 0;
}

DialogChangePassword::~DialogChangePassword() { delete ui; }

void DialogChangePassword::UpdatePasswordRetry() {
  QString noTrialsLeft;
  int retryCount = 0;
  // update password retry values
  if (TRUE == cryptostick->activStick20) {
    cryptostick->stick20GetStatusData();
    CheckResponse(FALSE);
  }
  cryptostick->getPasswordRetryCount();
  cryptostick->getUserPasswordRetryCount();
  switch (PasswordKind) {
  case STICK20_PASSWORD_KIND_USER:
  case STICK10_PASSWORD_KIND_USER:
    retryCount = HID_Stick20Configuration_st.UserPwRetryCount;
    noTrialsLeft = tr("Unfortunately you have no more trials left. Please use 'Reset User PIN' "
                      "option from menu to reset password");
    break;
  case STICK20_PASSWORD_KIND_ADMIN:
  case STICK10_PASSWORD_KIND_ADMIN:
  case STICK20_PASSWORD_KIND_RESET_USER:
  case STICK10_PASSWORD_KIND_RESET_USER:
    retryCount = HID_Stick20Configuration_st.AdminPwRetryCount;
    noTrialsLeft = tr("Unfortunately you have no more trials left. Please check instruction how to "
                      "reset Admin password.");
    break;
  case STICK20_PASSWORD_KIND_UPDATE:
    // FIXME add firmware counter
    retryCount = 99;
    ui->retryCount->hide();
    ui->retryCountLabel->hide();
    break;
  }
  if (retryCount == 0) {
    csApplet()->warningBox(noTrialsLeft);
    QString cssRed = "QLabel {color: red; font-weight: bold}";
    ui->retryCount->setStyleSheet(cssRed);
    ui->retryCountLabel->setStyleSheet(cssRed);
    done(true);
  }
  ui->retryCount->setText(QString::number(retryCount));
  ui->retryCount->repaint();
}

void DialogChangePassword::UI_deviceNotInitialized() const { csApplet()->warningBox(tr("Device is not yet initialized. Please try again later.")); }


int DialogChangePassword::exec() {
  if (!cryptostick->isInitialized()) {
    UI_deviceNotInitialized();
    done(Rejected);
    return QDialog::Rejected;
  }
  return QDialog::exec();
}

void DialogChangePassword::InitData(void) {
  // center the password window
  QDesktopWidget *desktop = QApplication::desktop();
  setGeometry(
      QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), desktop->availableGeometry()));
  // replace %1 and %2 from text with proper values
  //(min and max of password length)
  int minimumPasswordLengthAdmin = 8;
  QString text = ui->label_additional_information->text();
  text = text.arg(minimumPasswordLength).arg(STICK20_PASSOWRD_LEN).arg(minimumPasswordLengthAdmin);
  if (PasswordKind == STICK20_PASSWORD_KIND_UPDATE) {
    text += QString("<i>") + tr("Once the firmware password is forgotten the Nitrokey can't be "
                                "updated or reset. Don't lose your firmware password, please.") +
            QString("</i>");
  }
  ui->label_additional_information->setText(text);

  this->UpdatePasswordRetry();

  switch (PasswordKind) {
  case STICK20_PASSWORD_KIND_USER:
  case STICK10_PASSWORD_KIND_USER:
    this->setWindowTitle(tr("Set User PIN"));
    ui->label_2->setText(tr("Current User PIN:"));
    ui->label_3->setText(tr("New User PIN:"));
    ui->label_4->setText(tr("New User PIN:"));
    break;
  case STICK20_PASSWORD_KIND_ADMIN:
  case STICK10_PASSWORD_KIND_ADMIN:
    this->setWindowTitle(tr("Set Admin PIN"));
    ui->label_2->setText(tr("Current Admin PIN:"));
    ui->label_3->setText(tr("New Admin PIN:"));
    ui->label_4->setText(tr("New Admin PIN:"));
    break;
  case STICK20_PASSWORD_KIND_RESET_USER:
  case STICK10_PASSWORD_KIND_RESET_USER:
    this->setWindowTitle(tr("Reset User PIN"));
    ui->label_2->setText(tr("Admin PIN:"));
    ui->label_3->setText(tr("New User PIN:"));
    ui->label_4->setText(tr("New User PIN:"));
    break;
  case STICK20_PASSWORD_KIND_UPDATE:
    this->setWindowTitle(tr("Change Firmware Password"));
    ui->label_2->setText(tr("Current Firmware Password:"));
    ui->label_3->setText(tr("New Firmware Password:"));
    ui->label_4->setText(tr("New Firmware Password:"));
    ui->checkBox->setText(tr("Show password"));
    break;
  }
}

int DialogChangePassword::CheckResponse(bool NoStopFlag) {
  Stick20ResponseTask ResponseTask(this, cryptostick, NULL);
  if (FALSE == NoStopFlag) {
    ResponseTask.NoStopWhenStatusOK();
  }
  ResponseTask.GetResponse();
  return (ResponseTask.ResultValue);
}

bool DialogChangePassword::SendNewPassword(void) {
  bool communicationSuccess;
  QByteArray PasswordString;
  constexpr int password_length = STICK20_PASSOWRD_LEN;
  unsigned char Data[password_length + 2];

  // Set kind of password
  switch (PasswordKind) {
  case STICK20_PASSWORD_KIND_USER:
    Data[0] = 'P';
    break;
  case STICK20_PASSWORD_KIND_ADMIN:
    Data[0] = 'A';
    break;
  default:
    Data[0] = '?';
    break;
  }

  // Send old password
  PasswordString = ui->lineEdit_OldPW->text().toLatin1();

  STRNCPY((char *)&Data[1], STICK20_PASSOWRD_LEN - 1, PasswordString.data(), STICK20_PASSOWRD_LEN);
  Data[STICK20_PASSOWRD_LEN + 1] = 0;

  communicationSuccess = cryptostick->stick20SendPassword(Data);
  if (!communicationSuccess) {
    csApplet()->warningBox(tr("There was a problem during communicating with device. Please retry."));
    return false;
  }

  bool isOldPasswordCorrect = CheckResponse(TRUE) == 1;
  if (!isOldPasswordCorrect) {
    csApplet()->warningBox(tr("Current password is not correct. Please retry."));
    return false;
  }

  // Change password
  PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();

  STRNCPY((char *)&Data[1], STICK20_PASSOWRD_LEN, PasswordString.data(), STICK20_PASSOWRD_LEN);
  Data[STICK20_PASSOWRD_LEN + 1] = 0;

  communicationSuccess = cryptostick->stick20SendNewPassword(Data);
  if (!communicationSuccess) {
    csApplet()->warningBox(tr("There was a problem during communicating with device. Please retry."));
    return false;
  }

  bool isNewPasswordCorrect = CheckResponse(FALSE) == 1;
  if (!isNewPasswordCorrect) {
    csApplet()->warningBox(tr("New password is not correct. Please retry."));
    return false;
  }
  csApplet()->messageBox(tr("New password is set"));
  return true;
}

bool DialogChangePassword::Stick10ChangePassword(void) {
  int ret;
  constexpr int password_length = STICK10_PASSWORD_LEN;
  QByteArray PasswordString;
  unsigned char old_pin[password_length + 1];
  unsigned char new_pin[password_length + 1];

  memset(old_pin, 0, sizeof(old_pin));
  memset(new_pin, 0, sizeof(new_pin));

  PasswordString = ui->lineEdit_OldPW->text().toLatin1();
  strncpy((char *)old_pin, PasswordString.data(), password_length);

  PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();
  strncpy((char *)new_pin, PasswordString.data(), password_length);

  // Change password
  if (PasswordKind == STICK10_PASSWORD_KIND_ADMIN)
    ret = cryptostick->changeAdminPin(old_pin, new_pin);
  else // if ( PasswordKind == STICK10_PASSWORD_KIND_USER )
    ret = cryptostick->changeUserPin(old_pin, new_pin);

  if (ret == CMD_STATUS_WRONG_PASSWORD) {
    csApplet()->warningBox(tr("Wrong password."));
  } else if (ret != CMD_STATUS_OK) {
    csApplet()->warningBox(tr("Couldn't change %1 password")
                                   .arg((PasswordKind == STICK10_PASSWORD_KIND_USER) ? "user" : "admin"));
  }
  bool success = ret == CMD_STATUS_OK;
  if (success) {
    csApplet()->messageBox(tr("New password is set"));
  }
  return success;
}

// FIXME code doubles SendNewPassword
bool DialogChangePassword::ResetUserPassword(void) {
  bool communicationAndCommandSuccess;
  QByteArray PasswordString;
  unsigned char Data[STICK20_PASSOWRD_LEN + 2];

  // Set kind of password
  Data[0] = 'A';
  // Send old password
  PasswordString = ui->lineEdit_OldPW->text().toLatin1();
  STRNCPY((char *)&Data[1], STICK20_PASSOWRD_LEN - 1, PasswordString.data(), STICK20_PASSOWRD_LEN);
  Data[STICK20_PASSOWRD_LEN + 1] = 0;

  communicationAndCommandSuccess = cryptostick->stick20SendPassword(Data);
  if (!communicationAndCommandSuccess) {
    csApplet()->warningBox(tr("There was a problem during communicating with device. Please retry."));
    return false;
  }

  bool isAdminPasswordCorrect = CheckResponse(TRUE) == 1;
  if (!isAdminPasswordCorrect) {
    csApplet()->warningBox(tr("Current Admin password is not correct. Please retry."));
    return false;
  }

  // Reset new user PIN
  PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();
  STRNCPY((char *)&Data[1], STICK20_PASSOWRD_LEN - 1, PasswordString.data(), STICK20_PASSOWRD_LEN);
  Data[STICK20_PASSOWRD_LEN + 1] = 0;
  communicationAndCommandSuccess = cryptostick->unlockUserPassword(Data) == 0;
  if (!communicationAndCommandSuccess) {
    csApplet()->warningBox(tr("There was a problem during communicating with device or new password "
                                      "is not correct. Please retry."));
    return false;
  }

  csApplet()->messageBox(tr("New User password is set"));
  return true;
}

bool DialogChangePassword::ResetUserPasswordStick10(void) {
  int ret;
  QByteArray PasswordString;
  unsigned char data[50 + 1];

  memset(data, 0, sizeof(data));

  // New User PIN
  PasswordString = ui->lineEdit_OldPW->text().toLatin1();
  STRNCPY((char *)data, 25, PasswordString.data(), 25);

  // Admin PIN
  PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();
  STRNCPY((char *)&(data[25]), 25, PasswordString.data(), 25);

  ret = cryptostick->unlockUserPasswordStick10(data);

  bool success = ret == CMD_STATUS_OK;
  if (!success) {
    if (CMD_STATUS_WRONG_PASSWORD == ret) {
      csApplet()->warningBox(tr("Wrong Admin PIN."));
    } else {
      csApplet()->warningBox(tr("Couldn't unblock the user PIN. Error: %1").arg(ret));
    }
  } else {
    csApplet()->messageBox(tr("User PIN successfully unblocked"));
  }
  return success;
}

bool DialogChangePassword::Stick20ChangeUpdatePassword(void) {
  bool commandSuccess;
  QByteArray PasswordString;
  constexpr int password_length = CS20_MAX_UPDATE_PASSWORD_LEN;
  unsigned char old_pin[password_length + 1];
  unsigned char new_pin[password_length + 1];

  memset(old_pin, 0, password_length + 1);
  memset(new_pin, 0, password_length + 1);

  PasswordString = ui->lineEdit_OldPW->text().toLatin1();
  strncpy((char *)old_pin, PasswordString.data(), password_length);

  PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();
  strncpy((char *)new_pin, PasswordString.data(), password_length);

  // Change password
  commandSuccess = cryptostick->stick20NewUpdatePassword((uint8_t *)old_pin, (uint8_t *)new_pin);

  if (!commandSuccess) {
    csApplet()->warningBox(tr("Wrong password or there was a communication problem with the device."));
    return false;
  }
  csApplet()->warningBox(tr("Password has been changed with success!"));
  return true;
}

void DialogChangePassword::accept() {
  // Check the length of the old password
  if (minimumPasswordLength > strlen(ui->lineEdit_OldPW->text().toLatin1())) {
    clearFields();
    QString OutputText;

    OutputText = tr("The minimum length of the old password is ") +
                 QString("%1").arg(minimumPasswordLength) + tr(" chars");

    csApplet()->warningBox(OutputText);
    return;
  }

  // Check for correct new password entrys
  if (0 !=
      strcmp(ui->lineEdit_NewPW_1->text().toLatin1(), ui->lineEdit_NewPW_2->text().toLatin1())) {
    clearFields();
    csApplet()->warningBox(tr("The new password entrys are not the same"));
    return;
  }

  // Check the new length of password - max
  // obsolete since input field max length is set, but should be checked
  // nevertheless in case STICK20_PASSOWRD_LEN would be changed to lower value
  if (STICK20_PASSOWRD_LEN < strlen(ui->lineEdit_NewPW_1->text().toLatin1())) {
    clearFields();
    QString OutputText;

    OutputText = tr("The maximum length of a password is ") +
                 QString("%1").arg(STICK20_PASSOWRD_LEN) + tr(" chars");

    csApplet()->warningBox(OutputText);
    return;
  }

  // Check the new length of password - min
  if (minimumPasswordLength > strlen(ui->lineEdit_NewPW_1->text().toLatin1())) {
    clearFields();
    QString OutputText;

    OutputText = tr("The minimum length of a password is ") +
                 QString("%1").arg(minimumPasswordLength) + tr(" chars");

    csApplet()->warningBox(OutputText);
    return;
  }

  bool success = false;
  // Send password to Stick 2.0
  switch (PasswordKind) {
  case STICK20_PASSWORD_KIND_USER:
  case STICK20_PASSWORD_KIND_ADMIN:
    success = SendNewPassword();
    break;
  case STICK10_PASSWORD_KIND_USER:
  case STICK10_PASSWORD_KIND_ADMIN:
    success = Stick10ChangePassword();
    break;
  case STICK20_PASSWORD_KIND_RESET_USER:
    success = ResetUserPassword();
    break;
  case STICK10_PASSWORD_KIND_RESET_USER:
    success = ResetUserPasswordStick10();
    break;
  case STICK20_PASSWORD_KIND_UPDATE:
    success = Stick20ChangeUpdatePassword();
    break;
  default:
    break;
  }

  cryptostick->getStatus();

  if (success) {
    done(true);
    return;
  }
  this->UpdatePasswordRetry();
  this->clearFields();
}

/*******************************************************************************

  on_checkBox_clicked

  Set echo mode for password edit

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void DialogChangePassword::on_checkBox_clicked(bool checked) {
  if (checked) {
    ui->lineEdit_OldPW->setEchoMode(QLineEdit::Normal);
    ui->lineEdit_NewPW_1->setEchoMode(QLineEdit::Normal);
    ui->lineEdit_NewPW_2->setEchoMode(QLineEdit::Normal);
  } else {
    ui->lineEdit_OldPW->setEchoMode(QLineEdit::Password);
    ui->lineEdit_NewPW_1->setEchoMode(QLineEdit::Password);
    ui->lineEdit_NewPW_2->setEchoMode(QLineEdit::Password);
  }
}

void DialogChangePassword::clearFields() {
  ui->lineEdit_OldPW->clear();
  ui->lineEdit_NewPW_1->clear();
  ui->lineEdit_NewPW_2->clear();

  ui->lineEdit_OldPW->setFocus();
}
