/*
 * Copyright (c) 2017-2018 Nitrokey UG
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
#include <QDateTime>
#include <QMimeData>
#include <QSettings>
#include <QTimer>
#include <utility>

void Clipboard::copyOTP(QString text) {
  QSettings settings;
  copyToClipboard(std::move(text), settings.value("clipboard/OTP_time", 120).toInt());
}

void Clipboard::copyPWS(QString text) {
  QSettings settings;
  copyToClipboard(std::move(text), settings.value("clipboard/PWS_time", 60).toInt());
}

//  see https://github.com/keepassxreboot/keepassxc/blob/develop/src/gui/Clipboard.cpp#L55
void Clipboard::copyToClipboard(QString text, int time) {
  if (text.length() != 0) {
    secretInClipboard = std::move(text);
    lastClipboardTime = QDateTime::currentDateTimeUtc().toTime_t() + time - 1;

    auto *mime = new QMimeData;
    mime->setText(secretInClipboard);

    // select proper MIME string
#ifdef Q_OS_MACOS
    mime->setData("application/x-nspasteboard-concealed-type", text.toUtf8());
#endif
#ifdef Q_OS_LINUX
    mime->setData("x-kde-passwordManagerHint", QByteArrayLiteral("secret"));
#endif
#ifdef Q_OS_WIN
    mime->setData("ExcludeClipboardContentFromMonitorProcessing", QByteArrayLiteral("1"));
#endif
    clipboard->setMimeData(mime, QClipboard::Clipboard);

#ifndef Q_OS_MACOS
    if (clipboard->supportsSelection()) {
      clipboard->setMimeData(mime, QClipboard::Selection);
    }
#endif //! Q_OS_MACOS
  }
  QTimer::singleShot(time*1000, this, SLOT(checkClipboard_Valid()));
}

#include <core/SecureString.h>
void Clipboard::checkClipboard_Valid(bool force_clear) {
  uint64_t currentTime = QDateTime::currentDateTimeUtc().toTime_t();

  if (force_clear || (currentTime >= lastClipboardTime)) {
    if (clipboard->text() == secretInClipboard) {
      clipboard->clear();
    }
    overwrite_string(secretInClipboard);
  }
}

Clipboard::Clipboard(QObject *parent) : QObject(parent), lastClipboardTime(0) {
  clipboard = QApplication::clipboard();
}

Clipboard::~Clipboard() {
  checkClipboard_Valid(true);
}
