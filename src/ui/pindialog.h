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


#ifndef PINDIALOG_H
#define PINDIALOG_H

#include <src/libada.h>

static const int PIN_LENGTH_MAXIMUM = 30;

static const int PIN_LENGTH_MINIMUM = 6;

static const char *const DEFAULT_PIN_USER = "123456";

static const char *const DEFAULT_PIN_ADMIN = "12345678";

static const int UI_PASSWORD_LENGTH_MAXIMUM = STICK20_PASSOWRD_LEN;

#include <QDialog>
#include <QObject>
#include <QtCore/QThread>
#include "ui_pindialog.h"
#include <memory>

enum PinType { USER_PIN, ADMIN_PIN, HIDDEN_VOLUME, FIRMWARE_PIN, OTHER };

namespace Ui {
class PinDialog;
}

namespace PinDialogUI{
    class Worker : public QObject
    {
    Q_OBJECT
    public:
        Worker(PinType _pin_type);
        struct {
            int retry_user_count;
            int retry_admin_count;
        } devdata;
    private:
        PinType pin_type;
    public slots:
        void fetch_device_data();
    signals:
        void finished();
    };
}

class PinDialog : public QDialog {
  Q_OBJECT
public :
    explicit PinDialog(PinType pinType, QWidget *parent = nullptr);
  ~PinDialog();

  void getPassword(char *text);
  void getPassword(QString &pin);
  std::string getPassword();
  virtual int exec() override;

private slots:
  void on_checkBox_toggled(bool checked);
  void onOkButtonClicked();
    void updateTryCounter();

private:
    Q_DISABLE_COPY(PinDialog);
    std::shared_ptr<Ui::PinDialog> ui;
  const PinType _pinType;
  PinDialogUI::Worker worker;
  QThread worker_thread;
  static int instances;
  static PinDialog* current_instance;

  void clearBuffers();
  void UI_deviceNotInitialized() const;
};

#endif // PINDIALOG_H
