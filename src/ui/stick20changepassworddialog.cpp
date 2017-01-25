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
#include "nitrokey-applet.h"
#include "stick20responsedialog.h"
#include "ui_stick20changepassworddialog.h"
#include "libada.h"
#include "src/utils/bool_values.h"
#include "libnitrokey/include/NitrokeyManager.h"
using nm = nitrokey::NitrokeyManager;

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
  PasswordKind = 0;
}

DialogChangePassword::~DialogChangePassword() { delete ui; }

void DialogChangePassword::UpdatePasswordRetry() {
  QString noTrialsLeft;
  int retryCount = 0;
  // update password retry values

  switch (PasswordKind) {
  case STICK20_PASSWORD_KIND_USER:
  case STICK10_PASSWORD_KIND_USER:
    retryCount = libada::i()->getUserPasswordRetryCount();
    noTrialsLeft = tr("Unfortunately you have no more trials left. Please use 'Reset User PIN' "
                      "option from menu to reset password");
    break;
  case STICK20_PASSWORD_KIND_ADMIN:
  case STICK10_PASSWORD_KIND_ADMIN:
  case STICK20_PASSWORD_KIND_RESET_USER:
  case STICK10_PASSWORD_KIND_RESET_USER:
    retryCount = libada::i()->getPasswordRetryCount();
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

void DialogChangePassword::moveWindowToCenter() {// center the password window
  QDesktopWidget *desktop = QApplication::desktop();
  setGeometry(
      QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), desktop->availableGeometry()));
}

int DialogChangePassword::CheckResponse(bool NoStopFlag) {
//  Stick20ResponseTask ResponseTask(this, cryptostick, NULL);
//  if (FALSE == NoStopFlag) {
//    ResponseTask.NoStopWhenStatusOK();
//  }
//  ResponseTask.GetResponse();
//  return (ResponseTask.ResultValue);
  //TODO implement waiting for results
}

bool DialogChangePassword::_changePassword(void) {
  QByteArray PasswordString, PasswordStringNew;
  PasswordString = ui->lineEdit_OldPW->text().toLatin1();
  PasswordStringNew = ui->lineEdit_NewPW_1->text().toLatin1();

  try {
    switch (PasswordKind) {
      case STICK20_PASSWORD_KIND_USER:
        nm::instance()->change_user_PIN(PasswordString.constData(), PasswordStringNew.constData());
        break;
      case STICK20_PASSWORD_KIND_ADMIN:
        nm::instance()->change_admin_PIN(PasswordString.constData(), PasswordStringNew.constData());
        break;
      case STICK20_PASSWORD_KIND_UPDATE:
        nm::instance()->change_update_password(PasswordString.constData(), PasswordStringNew.constData());
        break;
      case STICK20_PASSWORD_KIND_RESET_USER:
        nm::instance()->unlock_user_password(PasswordString.constData(), PasswordStringNew.constData());
        break;
      default:
        throw std::runtime_error("Not supported");
        break;
    }
    csApplet()->messageBox(tr("New password is set"));
    return true;
  }
  catch (CommandFailedException &e){
    if(!e.reason_wrong_password())
      throw;
    csApplet()->warningBox(tr("Current password is not correct. Please retry."));
    return false;
  }

  return false;
}


bool DialogChangePassword::SendNewPassword(void) {
  return _changePassword();
}

//
//// FIXME code doubles SendNewPassword
//bool DialogChangePassword::ResetUserPassword(void) {
//  QByteArray PasswordString, PasswordStringNewUser;
//  PasswordString = ui->lineEdit_OldPW->text().toLatin1();
//  PasswordStringNewUser = ui->lineEdit_NewPW_1->text().toLatin1();
//
//  try{
//    nm::instance()->unlock_user_password(PasswordString.constData(),
//                                         PasswordStringNewUser.constData());
////    csApplet()->messageBox(tr("User PIN successfully unblocked"));
//    csApplet()->messageBox(tr("New User password is set"));
//    return true;
//  }
//  catch (CommandFailedException &e){
//    if(!e.reason_wrong_password()){
//      csApplet()->warningBox(tr("There was a problem during communicating with device. Please retry."));
//      throw;
//      return false;
//    }
//    csApplet()->warningBox(tr("Current Admin password is not correct. Please retry."));
//  }
//  return false;
//}
//
//bool DialogChangePassword::Stick20ChangeUpdatePassword(void) {
//  QByteArray PasswordString = ui->lineEdit_OldPW->text().toLatin1();
//  QByteArray PasswordStringNew = ui->lineEdit_NewPW_1->text().toLatin1();
//  try{
//    nm::instance()->change_update_password(PasswordString.constData(), PasswordStringNew.constData());
//    csApplet()->warningBox(tr("Password has been changed with success!"));
//    return true;
//  }
//  catch (CommandFailedException &e){
//    if(!e.reason_wrong_password()){
//      csApplet()->warningBox(tr("There was a problem during communicating with device. Please retry."));
//      throw;
//      return false;
//    }
//  }
//  return false;
//}

void DialogChangePassword::accept() {
  // Check the length of the old password
  if (minimumPasswordLength > ui->lineEdit_OldPW->text().toLatin1().length()) {
    clearFields();
    QString OutputText;

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
  // obsolete since input field max length is set, but should be checked
  // nevertheless in case STICK20_PASSOWRD_LEN would be changed to lower value
  if (STICK20_PASSOWRD_LEN < ui->lineEdit_NewPW_1->text().toLatin1().length()) {
    clearFields();
    QString OutputText;

    OutputText = tr("The maximum length of a password is ") +
                 QString("%1").arg(STICK20_PASSOWRD_LEN) + tr(" chars");

    csApplet()->warningBox(OutputText);
    return;
  }

  // Check the new length of password - min
  if (minimumPasswordLength > ui->lineEdit_NewPW_1->text().toLatin1().length()) {
    clearFields();
    QString OutputText;

    OutputText = tr("The minimum length of a password is ") +
                 QString("%1").arg(minimumPasswordLength) + tr(" chars");

    csApplet()->warningBox(OutputText);
    return;
  }

//  bool success = false;
  // Send password to Stick 2.0
//  switch (PasswordKind) {
//  case STICK20_PASSWORD_KIND_USER:
//  case STICK20_PASSWORD_KIND_ADMIN:
//  case STICK10_PASSWORD_KIND_USER:
//  case STICK10_PASSWORD_KIND_ADMIN:
//    success = SendNewPassword();
//    break;
//  case STICK20_PASSWORD_KIND_RESET_USER:
//  case STICK10_PASSWORD_KIND_RESET_USER:
//    success = ResetUserPassword();
//    break;
//  case STICK20_PASSWORD_KIND_UPDATE:
//    success = Stick20ChangeUpdatePassword();
//    break;
//  default:
//    break;
//  }
  bool success = _changePassword();

  if (success) {
    done(true);
    return;
  }
  this->UpdatePasswordRetry();
  this->clearFields();
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
