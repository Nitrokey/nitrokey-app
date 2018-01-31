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

#include <QDialog>
#include <QObject>
#include <QtCore/QThread>
#include "ui_pindialog.h"
#include <memory>

namespace Ui {
class PinDialog;
}

namespace PinDialogUI{
    class Worker : public QObject
    {
    Q_OBJECT
    public:
        struct {
            int retry_user_count;
            int retry_admin_count;
        } devdata;
    public slots:
        void fetch_device_data();
    signals:
        void finished();
    };
}

class PinDialog : public QDialog {
  Q_OBJECT
public :
    enum PinType { USER_PIN, ADMIN_PIN, HIDDEN_VOLUME, FIRMWARE_PIN, OTHER };
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
  PinDialogUI::Worker worker;
  QThread worker_thread;

  const PinType _pinType;

  void clearBuffers();
  void UI_deviceNotInitialized() const;
};

#endif // PINDIALOG_H
