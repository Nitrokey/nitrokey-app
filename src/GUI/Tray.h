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

#ifndef NITROKEYAPP_TRAY_H
#define NITROKEYAPP_TRAY_H

#define TRAY_MSG_TIMEOUT 5000

#include <QObject>
enum trayMessageType { INFORMATION, WARNING, CRITICAL };

namespace Ui {
    class Tray;
}

#include <QMutex>
class tray_Worker : public QObject
{
Q_OBJECT

public:
    QMutex mtx;

public slots:
    void doWork();

signals:
    void resultReady();
    void progress(int i);
};

#include <QSystemTrayIcon>
#include <QAction>
#include <memory>
#include <QtWidgets/QMainWindow>
#include "StorageActions.h"

class Tray : public QObject {
Q_OBJECT


public:
  void setAdmin_mode(bool _admin_mode);
  void setDebug_mode(bool _debug_mode);
  Tray(QObject * _parent, bool _debug_mode, bool _extended_config, StorageActions *actions);
    virtual ~Tray();
    void showTrayMessage(const QString& message);
    void showTrayMessage(const QString &title, const QString &msg, enum trayMessageType type = INFORMATION,
                         int timeout = TRAY_MSG_TIMEOUT);
  public slots:
    void regenerateMenu();
    void updateOperationInProgressBar(int p);

  signals:
    void progress(int i);


private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void generateMenuPasswordSafe(std::shared_ptr<QMenu> &trayMenu, std::shared_ptr<QMenu> &windowMenu);
    void populateOTPPasswordMenu();
    void passOTPProgressFurther(int i);
    void showOTPProgressInTray(int i);
    void delayedShowIndicator();

private:
    Q_DISABLE_COPY(Tray);
    virtual bool eventFilter(QObject *obj, QEvent *event) override;
    void generateMenu(bool init=false, std::function<void(QMenu *)> run_before = [](QMenu*){});
    void generateMenuForStorageDevice(std::shared_ptr<QMenu> trayMenu, std::shared_ptr<QMenu> windowMenu);
    void generatePasswordMenu(std::shared_ptr<QMenu> &trayMenu, std::shared_ptr<QMenu> &windowMenu);
    void generateMenuForProDevice(std::shared_ptr<QMenu> &trayMenu, std::shared_ptr<QMenu> &windowMenu);
    void initActionsForStick10();
    void initActionsForStick20();
    void initCommonActions();
    int UpdateDynamicMenuEntrys(void);
    void createIndicator();

    QObject *main_window;
    QObject *storageActions;
    bool debug_mode;
    bool ExtendedConfigActive;

    QSystemTrayIcon *trayIcon;
    std::shared_ptr<QMenu> trayMenu;
    std::shared_ptr<QMenu> windowMenu;
    QMenu *trayMenuSubConfigure;
    std::shared_ptr<QMenu> trayMenuPasswdSubMenu;
//    QMenu *trayMenuTOTPSubMenu;
//    QMenu *trayMenuHOTPSubMenu;
    QMenu *trayMenuSubSpecialConfigure;
    QMenuBar* file_menu;
    QTimer *delayedShowTimer;
public:
  void setFile_menu(QMenuBar *file_menu);

private:

  bool long_operation_in_progress = false;
  QAction *quitAction;
    QAction *configureAction;
    QAction *resetAction;
    QAction *configureActionStick20;
    QAction *DebugAction;
    QAction *ShowWindowAction;
    QAction *ActionAboutDialog;
    QAction *ActionHelp;
//    QAction *SecPasswordAction;
//    QAction *Stick20SetupAction;
    QAction *Stick10ActionChangeUserPIN;
    QAction *Stick10ActionChangeAdminPIN;
    QAction *LockDeviceAction;
    QAction *UnlockPasswordSafeAction;

    QAction *Stick20ActionEnableCryptedVolume;
    QAction *Stick20ActionDisableCryptedVolume;
    QAction *Stick20ActionEnableHiddenVolume;
    QAction *Stick20ActionDisableHiddenVolume;
    QAction *Stick20ActionChangeUserPIN;
    QAction *Stick20ActionChangeAdminPIN;
    QAction *Stick20ActionChangeUpdatePIN;
    QAction *Stick20ActionEnableFirmwareUpdate;
    QAction *Stick20ActionExportFirmwareToFile;
    QAction *Stick20ActionDestroyCryptedVolume;
    QAction *Stick20ActionInitCryptedVolume;
    QAction *Stick20ActionFillSDCardWithRandomChars;
    QAction *Stick20ActionGetStickStatus;
    QAction *Stick20ActionUpdateStickStatus;
    QAction *Stick20ActionSetReadonlyUncryptedVolume;
    QAction *Stick20ActionSetReadWriteUncryptedVolume;
    QAction *Stick20ActionDebugAction;
    QAction *Stick20ActionSetupHiddenVolume;
    QAction *Stick20ActionClearNewSDCardFound;
    QAction *Stick20ActionLockStickHardware;
    QAction *Stick20ActionResetUserPassword;
    QAction *Stick20ActionSetReadonlyEncryptedVolume;
    QAction *Stick20ActionSetReadWriteEncryptedVolume;

  void destroyThread();

  tray_Worker *worker;
  QSignalMapper *mapper_TOTP;
  QSignalMapper *mapper_HOTP;
  QSignalMapper *mapper_PWS;

  QAction *quitAction_tray;
  QAction *ActionHelp_tray;
  QAction *ActionAboutDialog_tray;
  QAction *Stick20ActionEnableCryptedVolume_tray;
  QAction *Stick20ActionDisableCryptedVolume_tray;
  QAction *Stick20ActionEnableHiddenVolume_tray;
  QAction *Stick20ActionDisableHiddenVolume_tray;
  std::shared_ptr<QMenu> trayMenuPasswdSubMenu_tray;
  QAction *UnlockPasswordSafeAction_tray;
};


#endif //NITROKEYAPP_TRAY_H
