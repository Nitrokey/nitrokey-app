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

#include "Clipboard.h"
#include <QApplication>
#include <QTimer>
#include <QDateTime>

void Clipboard::copyToClipboard(QString text, int time) {
  //TODO introduce longer time for OTP secrets: 120 seconds
  if (text.length() != 0) {
    lastClipboardTime = QDateTime::currentDateTime().toTime_t() + time - 1;
    clipboard->setText(text);
    secretInClipboard = text;
  }
  QTimer::singleShot(time*1000, this, SLOT(checkClipboard_Valid()));
}

#include <core/SecureString.h>
void Clipboard::checkClipboard_Valid(bool force_clear) {
  uint64_t currentTime = QDateTime::currentDateTime().toTime_t();

  if (force_clear || (currentTime >= lastClipboardTime)) {
    if (clipboard->text() == secretInClipboard) {
      clipboard->setText(QString(""));
      overwrite_string(secretInClipboard);
    }
  }
}

Clipboard::Clipboard(QObject *parent) : QObject(parent) {
  clipboard = QApplication::clipboard();
}

Clipboard::~Clipboard() {
  checkClipboard_Valid(true);
}
