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

#ifndef PINDIALOG_H
#define PINDIALOG_H

#include <QDialog>

#include "ui_pindialog.h"

namespace Ui {
class PinDialog;
}

class PinDialog : public QDialog {
  Q_OBJECT public : enum Usage { PLAIN, PREFIXED };
  enum PinType { USER_PIN, ADMIN_PIN, FIRMWARE_PIN, OTHER };

  explicit PinDialog(const QString &title, const QString &label, Usage usage, PinType pinType, QWidget *parent = nullptr);
  ~PinDialog();

  // void init(char *text,int RetryCount);
  void getPassword(char *text);
  void getPassword(QString &pin);
  std::string && getPassword();

  virtual int exec() override;

private slots:
  void on_checkBox_toggled(bool checked);

  void on_checkBox_PasswordMatrix_toggled(bool checked);

  void onOkButtonClicked();

private:
  Ui::PinDialog *ui;

  Usage _usage;
  PinType _pinType;

  void updateTryCounter();
  void clearBuffers();
  void UI_deviceNotInitialized() const;
};

#endif // PINDIALOG_H
