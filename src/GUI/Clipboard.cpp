//
// Created by sz on 16.01.17.
//

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
