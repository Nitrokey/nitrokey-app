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


#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QMutex>
#include <QThread>
#include <memory>
#include "src/utils/bool_values.h"

#include "src/version.h"

namespace Ui {
class AboutDialog;
}

namespace AboutDialogUI{
    class Worker : public QObject
    {
    Q_OBJECT
    public:
        QMutex mutex;
        struct {
            int majorFirmwareVersion;
            int minorFirmwareVersion;
            std::string cardSerial;
            int passwordRetryCount;
            int userPasswordRetryCount;
          struct {
            bool active;
            struct {
                bool plain;
                bool encrypted;
                bool hidden;
                bool plain_RW;
              bool encrypted_RW;
              bool hidden_RW;
            } volume_active;
            struct{
              int size_GB = 0;
              bool is_new;
              bool filled_with_random;
              uint32_t id;
            } sdcard;
            bool keys_initiated;
            bool firmware_locked;
            uint32_t smartcard_id;
            struct {
              bool status = false;
              int progress = 0;
            } long_operation;
          } storage;
          bool comm_error = false;
        } devdata;
    public slots:
        void fetch_device_data();
    signals:
        void finished(bool connected);
    };
}

class AboutDialog : public QDialog {
  Q_OBJECT
  public :
  explicit AboutDialog(QWidget *parent = 0);
  ~AboutDialog();
  void showStick20Configuration(void);
  void showStick10Configuration(void);
  void showNoStickFound(void);
  void hideStick20Menu(void);
  void showStick20Menu(void);
  void hidePasswordCounters(void);
  void showPasswordCounters(void);
  void hideWarning(void);
  void showWarning(void);

private slots:
  void on_ButtonOK_clicked();
  void on_ButtonStickStatus_clicked();
  void update_device_slots(bool connected);
  void on_btn_3rdparty_clicked();

private:
  std::shared_ptr<Ui::AboutDialog> ui;
  AboutDialogUI::Worker worker;
  QThread worker_thread;

  void fixWindowGeometry();

  void setStorageLabelsVisible(bool v);
};

#endif // ABOUTDIALOG_H
