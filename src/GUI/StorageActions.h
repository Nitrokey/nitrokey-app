//
// Created by sz on 23.01.17.
//

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
  void startLockDeviceAction();
  void startStick20SetReadOnlyUncryptedVolume();
  void startStick20SetReadWriteUncryptedVolume();
  void startStick20LockStickHardware();
  void startStick20EnableFirmwareUpdate();
  void startStick20ExportFirmwareToFile();
  void startStick20DebugAction();
  void startStick20ClearNewSdCardFound();
  void startStick20SetupHiddenVolume();

signals:
  void storageStatusChanged();
  void longOperationStarted();
  void FactoryReset();


};


#endif //NITROKEYAPP_STORAGEACTIONS_H
