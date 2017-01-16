/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *
 *
 * This file is part of Nitrokey.
 *
 * Nitrokey is free software: you can redistribute it and/or modify
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

#include "mcvs-wrapper.h"

#include "pindialog.h"

#include "nitrokey-applet.h"
#include "libada.h"
#include "src/utils/bool_values.h"

#define LOCAL_PASSWORD_SIZE 40 // Todo make define global

PinDialog::PinDialog(const QString &title, const QString &label, Usage usage, PinType pinType, QWidget *parent)
    : _usage(usage), _pinType(pinType), QDialog(parent),
      ui(new Ui::PinDialog) {
  ui->setupUi(this);

  connect(ui->okButton, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));

  // Setup pwd-matrix
  ui->checkBox_PasswordMatrix->setCheckState(Qt::Unchecked);
  ui->checkBox_PasswordMatrix->hide();

  // Setup title and label
  this->setWindowTitle(title);
  ui->label->setText(label);
  ui->lineEdit->setMaxLength(STICK20_PASSOWRD_LEN); // TODO change to
                                                    // UI_PASSWORD_LEN this and
                                                    // other occurences

  // ui->status->setVisible(false);
  updateTryCounter();

  ui->lineEdit->setFocus();
}

PinDialog::~PinDialog() { delete ui; }

/*
   void PinDialog::init(char *text,int RetryCount) { char text1[20];

   text1[0] = 0; if (-1 != RetryCount) { SNPRINTF (text1,sizeof (text1)," (Tries
   left: %d)",RetryCount); } ui->label->setText(tr(text)+tr(text1)); } */

void PinDialog::getPassword(char *text) {
  STRCPY(text, LOCAL_PASSWORD_SIZE, (char *)password);
  clearBuffers();
  /*
     if (FALSE == ui->checkBox_PasswordMatrix->isChecked()) { STRCPY
     (&text[1],LOCAL_PASSWORD_SIZE-1,ui->lineEdit->text().toLatin1()); } else {
     STRCPY (text,LOCAL_PASSWORD_SIZE,(char*)password); } */
}

void PinDialog::getPassword(QString &pin) {
  pin = ui->lineEdit->text();
  clearBuffers();
}

std::string && PinDialog::getPassword() {
  std::string pin = ui->lineEdit->text().toStdString();
  clearBuffers();
  ui->lineEdit->setText(ui->lineEdit->placeholderText());
  return std::move(pin);
}

void PinDialog::on_checkBox_toggled(bool checked) {
  ui->lineEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

void PinDialog::on_checkBox_PasswordMatrix_toggled(bool checked) {
  ui->lineEdit->setDisabled(checked);
}

void PinDialog::onOkButtonClicked() {
  int n;

  QByteArray passwordString;

  // Initialize password
  memset(password, 0, 50);

  if (false == ui->checkBox_PasswordMatrix->isChecked()) {
    // Send normal password
    if (PREFIXED == _usage) {
      password[0] = 'P';
    }

    // Check the password length
    passwordString = ui->lineEdit->text().toLatin1();
    n = passwordString.size();
    if (30 <= n) // FIXME use constants/defines!
    {
        csApplet()->warningBox(tr("Your PIN is too long! Use not more than 30 characters."));
      ui->lineEdit->clear();
      return;
    }
    if (6 > n) {
        csApplet()->warningBox(tr("Your PIN is too short. Use at least 6 characters."));
      ui->lineEdit->clear();
      return;
    }

    // Check for default pin
    if ((0 == strcmp(passwordString, "123456")) || (0 == strcmp(passwordString, "12345678"))) {
        csApplet()->warningBox(tr("Warning: Default PIN is used.\nPlease change the PIN."));
    }

    if (PREFIXED == _usage) {
      memcpy(&password[1], passwordString.data(), n);
    } else {
      memcpy(password, passwordString.data(), n);
    }
  }

  done(Accepted);
}

int PinDialog::exec() {
  if (!libada::i()->isDeviceInitialized()) {
        UI_deviceNotInitialized();
        done(Rejected);
        return QDialog::Rejected;
  }
  return QDialog::exec();
}

void PinDialog::updateTryCounter() {
  int triesLeft = 0;

  switch (_pinType) {
  case ADMIN_PIN:
    triesLeft = libada::i()->getPasswordRetryCount();
    break;
  case USER_PIN:
    triesLeft = libada::i()->getUserPasswordRetryCount();
    break;
  case FIRMWARE_PIN:
  case OTHER:
    // Hide tries left field
    ui->status->setVisible(false);
    break;
  }

  // Update 'tries-left' field
  ui->status->setText(tr("Tries left: %1").arg(triesLeft));
}

void PinDialog::UI_deviceNotInitialized() const { csApplet()->warningBox(tr("Device is not yet initialized. Please try again later.")); }

void PinDialog::clearBuffers() {
  memset(password, 0, 50);
  ui->lineEdit->clear();
}
