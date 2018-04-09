/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 * Copyright (c) 2012-2018 Nitrokey UG
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


#include "src/GUI/ManageWindow.h"
#include "mcvs-wrapper.h"
#include "pindialog.h"
#include "nitrokey-applet.h"
#include "stick20changepassworddialog.h"


int PinDialog::instances = 0;
PinDialog* PinDialog::current_instance = nullptr;

PinDialog::PinDialog(PinType pinType, QWidget *parent):
    QDialog(parent),
    _pinType(pinType), worker(pinType)
{
  ++instances;
  if (PinDialog::current_instance == nullptr)
     PinDialog::current_instance = this;

  ui = std::make_shared<Ui::PinDialog>();
  ui->setupUi(this);

  ui->status->setText(tr("Tries left: %1").arg("..."));
  QString title, label;
  switch (pinType){
    case USER_PIN:
      title = tr("Enter user PIN");
      label = tr("User PIN:");
      break;
    case ADMIN_PIN:
      title = tr("Enter admin PIN");
      label = tr("Admin PIN:");
      break;
    case FIRMWARE_PIN:
      title = tr("Enter Firmware Password");
      label = tr("Enter Firmware Password:");
      ui->status->setVisible(false);
      break;
    case OTHER:
        break;
    case HIDDEN_VOLUME:
      title = tr("Enter password for hidden volume");
      label = tr("Enter password for hidden volume:");
      ui->status->setVisible(false);
      break;
  }

//  worker.pin_type = _pinType;
  connect(&worker_thread, SIGNAL(started()), &worker, SLOT(fetch_device_data()));
  connect(&worker, SIGNAL(finished()), this, SLOT(updateTryCounter()));
  worker.moveToThread(&worker_thread);
  worker_thread.start();

  connect(ui->okButton, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));

  // Setup title and label
  this->setWindowTitle(title);
  ui->label->setText(label);
  ui->lineEdit->setAccessibleName(label);
  ui->lineEdit->setMaxLength(UI_PASSWORD_LENGTH_MAXIMUM);

  // ui->status->setVisible(false);

  ui->lineEdit->setFocus();

  this->setModal(true);  
}

PinDialog::~PinDialog() {
  worker_thread.quit();
  worker_thread.wait();
  --instances;
  if (PinDialog::current_instance == this)
    PinDialog::current_instance = nullptr;
}

void PinDialog::getPassword(QString &pin) {
  pin = ui->lineEdit->text();
  clearBuffers();
}

std::string PinDialog::getPassword() {
  std::string pin = ui->lineEdit->text().toStdString();
  clearBuffers();
  return pin;
}

void PinDialog::on_checkBox_toggled(bool checked) {
  ui->lineEdit->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
}

PasswordKind translate_type(PinType type){
  switch (type){
    case PinType::USER_PIN:
      return PasswordKind::USER;
    case PinType::ADMIN_PIN:
      return PasswordKind::ADMIN;
    case PinType::HIDDEN_VOLUME:break;
    case PinType::FIRMWARE_PIN:
      return PasswordKind::UPDATE;
    case PinType::OTHER:break;
  }
  throw std::runtime_error("Wrong password kind selected");
}

void PinDialog::onOkButtonClicked() {
  int n;

  // Check the password length
  auto passwordString = ui->lineEdit->text().toLatin1();
  n = passwordString.size();
  if (PIN_LENGTH_MAXIMUM <= n)
  {
      csApplet()->warningBox(tr("Your PIN is too long! Use not more than 30 characters.")); //FIXME use %1 instead of constant in translation
    ui->lineEdit->clear();
    return;
  }
  if (PIN_LENGTH_MINIMUM > n) {
      csApplet()->warningBox(tr("Your PIN is too short. Use at least 6 characters.")); //FIXME use %1 instead of constant in translation
    ui->lineEdit->clear();
    return;
  }

  // Check for default pin and ask for change if set default
  if (passwordString == DEFAULT_PIN_USER || passwordString == DEFAULT_PIN_ADMIN) {
    auto warning_message = tr("Warning: Default PIN is used.\nPlease change the PIN.");
    if (_pinType == USER_PIN || _pinType == ADMIN_PIN || _pinType == FIRMWARE_PIN) {
      auto yesOrNoBox = csApplet()->yesOrNoBox(warning_message +" "+ tr("Would you like to so now?"), true);
      if (yesOrNoBox) {
        auto dialog = new DialogChangePassword(this, translate_type(_pinType));
        dialog->InitData();
        const auto password_changed = dialog->exec();
        if (password_changed){
          ui->lineEdit->clear();
          csApplet()->messageBox(tr("Please enter the new PIN/password"));
        }
        return;
      }
    }
  }

  done(Accepted);
}

int PinDialog::exec() {
    if (PinDialog::instances > 1){
        done(Rejected);
        if (PinDialog::current_instance != nullptr){
            ManageWindow::bringToFocus(PinDialog::current_instance);
        }
        return QDialog::Rejected;
    }
  if (!libada::i()->isDeviceInitialized()) {
        UI_deviceNotInitialized();
        done(Rejected);
        return QDialog::Rejected;
  }
  ManageWindow::bringToFocus(this);
  return QDialog::exec();
}

void PinDialog::updateTryCounter() {
  int triesLeft = 0;

  switch (_pinType) {
  case ADMIN_PIN:
    triesLeft = worker.devdata.retry_admin_count;
    break;
  case USER_PIN:
    triesLeft = worker.devdata.retry_user_count;
    break;
  case HIDDEN_VOLUME:
  case FIRMWARE_PIN:
  case OTHER:
    // Hide tries left field
    ui->status->setVisible(false);
    break;
  }

  // Update 'tries-left' field
  ui->status->setText(tr("Tries left: %1").arg(triesLeft));
}

void PinDialog::UI_deviceNotInitialized() const {
  csApplet()->warningBox(tr("Device is not yet initialized. Please try again later."));
}

void PinDialog::clearBuffers() {
  //FIXME securely delete string in UI
  //FIXME make sure compiler will not ignore this
  ui->lineEdit->clear();
  ui->lineEdit->setText(ui->lineEdit->placeholderText());
}

void PinDialogUI::Worker::fetch_device_data() {
  try {
    if (pin_type == PinType::ADMIN_PIN){
      devdata.retry_admin_count = libada::i()->getAdminPasswordRetryCount();
    } else if (pin_type == PinType::USER_PIN) {
      devdata.retry_user_count = libada::i()->getUserPasswordRetryCount();
    }
    emit finished();
  }
  catch (...){
    //ignore
  }
}

PinDialogUI::Worker::Worker(const PinType _pin_type) {
  pin_type = _pin_type;
}

