/*
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

#include <QDateTime>
#include <QTimer>
#include "Authentication.h"
#include "pindialog.h"
#include "libada.h"
#include "libnitrokey/include/NitrokeyManager.h"
#include "nitrokey-applet.h"

using nm = nitrokey::NitrokeyManager;

static const int validation_period = 10 * 60 * 1000; //TODO move to field, add to constructor as a parameter

Authentication::Authentication(QObject *parent, Type _type) : QObject(parent), type(_type) {
}

std::string Authentication::getPassword() {
  const auto pinType = type==ADMIN? PinDialog::ADMIN_PIN : PinDialog::USER_PIN;
  PinDialog dialog(pinType);

  int ok = dialog.exec();
  if (ok != QDialog::Accepted) {
    return "";
  }

  return dialog.getPassword();
}

bool Authentication::authenticate(){
  bool authenticationSuccess = false;

  if (isAuthenticated()){
    authenticationSuccess = true;
    markAuthenticated();
    return authenticationSuccess;
  }

  QString password;
  do {
      const auto pinType = type==ADMIN? PinDialog::ADMIN_PIN : PinDialog::USER_PIN;
      PinDialog dialog(pinType);

    int ok = dialog.exec();
    if (ok != QDialog::Accepted) {
      return authenticationSuccess;
    }

    auto password = dialog.getPassword();
    tempPassword = generateTemporaryPassword();

    try{
      switch (pinType){
        case PinDialog::USER_PIN:
          nm::instance()->user_authenticate(password.c_str(), tempPassword.constData());
          break;
        case PinDialog::ADMIN_PIN:
          nm::instance()->first_authenticate(password.c_str(), tempPassword.constData());
          break;
        default:
          break;
      }

      markAuthenticated();
      authenticationSuccess = true;
      return authenticationSuccess;
    }
    catch (CommandFailedException &e){
      if (!e.reason_wrong_password())
        throw;
      csApplet()->warningBox(tr("Wrong PIN. Please try again."));
    }

  } while (true);
  return authenticationSuccess;
}

bool Authentication::isAuthenticated() const { return !tempPassword.isEmpty() && authenticationValidUntilTime >= getCurrentTime(); }

void Authentication::markAuthenticated() {
  authenticationValidUntilTime = getCurrentTime() + validation_period;
  // Arm the clearing of the temp password 1000 ms after the deadline to
  // avoid the race condition with comparing current time and expiry time.
  // On some OSes QTimer is rounded to nearest second hence need to add whole second.
  // See http://doc.qt.io/qt-5/qt.html#TimerType-enum
  QTimer::singleShot(validation_period + 1000, this, SLOT(clearTemporaryPassword()));
}

quint64 Authentication::getCurrentTime() const { return (quint64)QDateTime::currentMSecsSinceEpoch(); }

void Authentication::clearTemporaryPassword(bool force){
  if (tempPassword.isEmpty()) return;
  if (force || getCurrentTime() >= authenticationValidUntilTime) {
    tempPassword.clear(); //FIXME securely delete
  }
}


#include <random>
QByteArray Authentication::generateTemporaryPassword() const {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  static std::uniform_int_distribution<unsigned char> dist(0, 0xFF);

  auto temporary_password_length = 25;
  QByteArray tmp_p(temporary_password_length, 0);
  auto size = tmp_p.size();
  for (int i = 0; i < size; i++){
    tmp_p[i] = dist(mt);
  }
  return tmp_p;
}


const QByteArray Authentication::getTempPassword() {
    const QByteArray local_tempPassword = tempPassword;
    bool is_07nkpro_device = libada::i()->is_nkpro_07_rtm1();
    if (is_07nkpro_device) {
        clearTemporaryPassword(true);
    }
    return local_tempPassword;
}

Authentication::~Authentication() {
  tempPassword.clear();
}

void Authentication::clearTemporaryPasswordForced() {
  clearTemporaryPassword(true);
}
