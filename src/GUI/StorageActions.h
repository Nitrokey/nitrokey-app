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

#ifndef NITROKEYAPP_STORAGEACTIONS_H
#define NITROKEYAPP_STORAGEACTIONS_H
#include <QObject>
#include "src/GUI/Authentication.h"
#include "src/ui/nitrokey-applet.h"
#include <functional>

class StorageActions: public QObject {
Q_OBJECT

private:
  Q_DISABLE_COPY(StorageActions);

  Authentication * auth_admin;
  Authentication * auth_user;
  bool CryptedVolumeActive = false;
  bool HiddenVolumeActive = false;
  void _execute_SD_clearing(const std::string &s);
  std::function<void(QString)> startProgressFunc;
  std::function<void()> end_progress_function;
  std::function<void(QString)> show_message_function;
  void runAndHandleErrorsInUI(QString successMessage, QString operationFailureMessage,
                              std::function<void(void)> codeToRunInDeviceThread,
                              std::function<void(void)> onSuccessInGuiThread);
private slots:
  void on_StorageStatusChanged();

public:
  StorageActions(QObject *parent, Authentication *auth_admin, Authentication *auth_user);
  void set_start_progress_window(std::function<void(QString)> _start_progress_function);
  void set_end_progress_window(std::function<void()> _end_progress_function);
  void set_show_message(std::function<void(QString)> _show_message);

public slots:
  void startStick20DestroyCryptedVolume(int fillSDWithRandomChars);
  void startStick20FillSDCardWithRandomChars();
  void startStick20EnableCryptedVolume();
  void startStick20DisableCryptedVolume();
  void startStick20EnableHiddenVolume();
  void startStick20DisableHiddenVolume();
  void startLockDeviceAction(bool ask_for_confirmation = true);
  void startStick20SetReadOnlyUncryptedVolume();
  void startStick20SetReadWriteUncryptedVolume();
  void startStick20LockStickHardware();
  void startStick20EnableFirmwareUpdate();
  void startStick20ExportFirmwareToFile();

  void startStick20ClearNewSdCardFound();
  void startStick20SetupHiddenVolume();

signals:
  void storageStatusChanged();
  void storageStatusUpdated();
  void longOperationStarted();
  void FactoryReset();



};


#endif //NITROKEYAPP_STORAGEACTIONS_H
