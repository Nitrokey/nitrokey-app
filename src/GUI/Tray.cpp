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

/*
TODO
1. remove doubled debug
2. remove stick10 changing pin
*/

#include "Tray.h"
#include <QThread>
#include "libada.h"
#include "bool_values.h"
#include "nitrokey-applet.h"
#include <libnitrokey/NitrokeyManager.h>
#include <QMenu>
#include <QMenuBar>
#include "graphicstools.h"

Tray::Tray(QObject *_parent, bool _debug_mode, bool _extended_config,
           StorageActions *actions) :
    QObject(_parent),
    trayMenu(nullptr),
    trayMenuPasswdSubMenu(nullptr),
    file_menu(nullptr),
    worker(nullptr)
{
  main_window = _parent;
  storageActions = actions;
  debug_mode = _debug_mode;
  ExtendedConfigActive = _extended_config;

  createIndicator();
  initActionsForStick10();
  initActionsForStick20();
  initCommonActions();

  mapper_TOTP = new QSignalMapper(this);
  mapper_HOTP = new QSignalMapper(this);
  mapper_PWS = new QSignalMapper(this);
  connect(mapper_TOTP, SIGNAL(mapped(int)), main_window, SLOT(getTOTPDialog(int)));
  connect(mapper_HOTP, SIGNAL(mapped(int)), main_window, SLOT(getHOTPDialog(int)));
  connect(mapper_PWS, SIGNAL(mapped(int)), main_window, SLOT(PWS_ExceClickedSlot(int)));
}

void Tray::setDebug_mode(bool _debug_mode) {
  debug_mode = _debug_mode;
}

Tray::~Tray() {
  destroyThread();
}

void Tray::delayedShowIndicator(){
  if(!trayIcon->isSystemTrayAvailable()) return;
  trayIcon->show();
  delayedShowTimer->stop();
}

/*
 * Create the tray menu.
 */
void Tray::createIndicator() {
  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setIcon(GraphicsTools::loadColorize(":/images/new/icon_NK.svg", true, true));
  connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
          SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

  delayedShowTimer = new QTimer(this);
  connect(delayedShowTimer, SIGNAL(timeout()), this, SLOT(delayedShowIndicator()));
  delayedShowTimer->setSingleShot(false);
  delayedShowTimer->start(2000);

  // Initial message
  if (debug_mode)
    showTrayMessage("Nitrokey App", tr("Active (debug mode)"), INFORMATION, TRAY_MSG_TIMEOUT);
  else
    showTrayMessage("Nitrokey App", tr("Active"), INFORMATION, TRAY_MSG_TIMEOUT);
}

void Tray::showTrayMessage(QString message) {
  showTrayMessage("Nitrokey App", message, INFORMATION, 2000);
}

void Tray::showTrayMessage(const QString &title, const QString &msg,
                                 enum trayMessageType type, int timeout) {
  if(debug_mode)
    qDebug() << msg;
  if (trayIcon->supportsMessages()) {
    switch (type) {
      case INFORMATION:
        trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, timeout);
        break;
      case WARNING:
        trayIcon->showMessage(title, msg, QSystemTrayIcon::Warning, timeout);
        break;
      case CRITICAL:
        trayIcon->showMessage(title, msg, QSystemTrayIcon::Critical, timeout);
        break;
    }
  } else
    csApplet()->messageBox(msg);
}

void Tray::iconActivated(QSystemTrayIcon::ActivationReason reason) {

  switch (reason) {
    case QSystemTrayIcon::Context:
// trayMenu->close();
#ifdef Q_OS_MAC
      trayMenu->popup(QCursor::pos());
#endif
      break;
    case QSystemTrayIcon::Trigger:
#ifndef Q_OS_MAC
      trayMenu->popup(QCursor::pos());
#endif
      break;
    case QSystemTrayIcon::DoubleClick:
      break;
    case QSystemTrayIcon::MiddleClick:
      break;
    default:;
  }
}

bool Tray::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);

    if (mEvent->button() == Qt::LeftButton) {
      /*
         QMouseEvent my_event = new QMouseEvent ( mEvent->type(), mEvent->pos(),
         Qt::Rightbutton , mEvent->buttons(), mEvent->modifiers() );
         QCoreApplication::postEvent ( trayIcon, my_event ); */
      return true;
    }
  }
  return QObject::eventFilter(obj, event);
}

void Tray::generateMenu(bool init, std::function<void(QMenu *)> run_before) {
    static QMutex mtx;
    QMutexLocker locker(&mtx);

    std::shared_ptr<QMenu> trayMenuLocal;
    std::shared_ptr<QMenu> windowMenuLocal;

    windowMenuLocal = std::make_shared<QMenu>("Menu");
    trayMenuLocal = std::make_shared<QMenu>();

    run_before(trayMenuLocal.get());

    if (!init){
      // Setup the new menu
      if (!libada::i()->isDeviceConnected()) {
        trayMenuLocal->addAction(tr("Nitrokey is not connected!"));
      } else {
        if (!libada::i()->isStorageDeviceConnected()) // Nitrokey Pro connected
            generateMenuForProDevice(trayMenuLocal, windowMenuLocal);
        else {
          // Nitrokey Storage is connected
            generateMenuForStorageDevice(trayMenuLocal, windowMenuLocal);
        }
      }
    }

    // Add debug window ?
    if (debug_mode){
      trayMenuLocal->addAction(DebugAction);
      windowMenuLocal->addAction(DebugAction);
    }

    trayMenuLocal->addSeparator();

    if (!long_operation_in_progress)
        trayMenuLocal->addAction(ShowWindowAction);

    trayMenuLocal->addAction(ActionHelp_tray);
    trayMenuLocal->addAction(ActionAboutDialog_tray);
    trayMenuLocal->addAction(quitAction_tray);

    windowMenuLocal->addSeparator();
    windowMenuLocal->addAction(ActionHelp);
    windowMenuLocal->addAction(ActionAboutDialog);
    windowMenuLocal->addAction(quitAction);

    trayIcon->setContextMenu(trayMenuLocal.get());

    if (file_menu != nullptr && windowMenuLocal != nullptr){
//      file_menu->addMenu(windowMenuLocal.get()); // does not work for macOS
      file_menu->addAction(windowMenuLocal->menuAction());
    }

    trayMenu = trayMenuLocal;
    windowMenu = windowMenuLocal;
  }

void Tray::initActionsForStick10() {
  UnlockPasswordSafeAction = new QAction(tr("Unlock password safe"), main_window);
  UnlockPasswordSafeAction->setIcon(GraphicsTools::loadColorize(":/images/new/icon_safe.svg"));
  connect(UnlockPasswordSafeAction, SIGNAL(triggered()), main_window, SLOT(PWS_Clicked_EnablePWSAccess()));

  UnlockPasswordSafeAction_tray = new QAction(tr("Unlock password safe"), main_window);
  UnlockPasswordSafeAction_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_safe.svg", true));
  connect(UnlockPasswordSafeAction_tray, SIGNAL(triggered()), main_window, SLOT(PWS_Clicked_EnablePWSAccess()));

  configureAction = new QAction(tr("&OTP"), main_window);
  connect(configureAction, SIGNAL(triggered()), main_window, SLOT(startConfiguration()));

  resetAction = new QAction(tr("&Factory reset"), main_window);
  connect(resetAction, SIGNAL(triggered()), main_window, SLOT(factoryResetAction()));

  Stick10ActionChangeUserPIN = new QAction(tr("&Change User PIN"), main_window);
  connect(Stick10ActionChangeUserPIN, SIGNAL(triggered()), main_window,
          SLOT(startStick20ActionChangeUserPIN()));

  Stick10ActionChangeAdminPIN = new QAction(tr("&Change Admin PIN"), main_window);
  connect(Stick10ActionChangeAdminPIN, SIGNAL(triggered()), main_window,
          SLOT(startStick20ActionChangeAdminPIN()));
}

void Tray::initCommonActions() {
  DebugAction = new QAction(tr("&Debug"), main_window);
  connect(DebugAction, SIGNAL(triggered()), main_window, SLOT(startStickDebug()));

  ShowWindowAction = new QAction(tr("&Overview"), main_window);
  connect(ShowWindowAction, SIGNAL(triggered()), main_window, SLOT(startConfigurationMain()));

  quitAction_tray = new QAction(tr("&Quit"), main_window);
  quitAction_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_quit.svg", true));
  connect(quitAction_tray, SIGNAL(triggered()), qApp, SLOT(quit()));

  quitAction = new QAction(tr("&Quit"), main_window);
  quitAction->setIcon(GraphicsTools::loadColorize(":/images/new/icon_quit.svg"));
  connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

  ActionHelp = new QAction(tr("&Help"), main_window);
  ActionHelp->setIcon(GraphicsTools::loadColorize(":/images/new/icon_fragezeichen.svg"));
  connect(ActionHelp, SIGNAL(triggered()), main_window, SLOT(startHelpAction()));

  ActionHelp_tray = new QAction(tr("&Help"), main_window);
  ActionHelp_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_fragezeichen.svg", true));
  connect(ActionHelp_tray, SIGNAL(triggered()), main_window, SLOT(startHelpAction()));

  ActionAboutDialog = new QAction(tr("&About Nitrokey"), main_window);
  ActionAboutDialog->setIcon(GraphicsTools::loadColorize(":/images/new/icon_about_nitrokey.svg"));
  connect(ActionAboutDialog, SIGNAL(triggered()), main_window, SLOT(startAboutDialog()));

  ActionAboutDialog_tray = new QAction(tr("&About Nitrokey"), main_window);
  ActionAboutDialog_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_about_nitrokey.svg", true));
  connect(ActionAboutDialog_tray, SIGNAL(triggered()), main_window, SLOT(startAboutDialog()));
}

void Tray::initActionsForStick20() {
  configureActionStick20 = new QAction(tr("&OTP and Password safe"), main_window);
  connect(configureActionStick20, SIGNAL(triggered()), main_window, SLOT(startConfiguration()));

  Stick20ActionEnableCryptedVolume = new QAction(tr("&Unlock encrypted volume"), main_window);
  Stick20ActionEnableCryptedVolume->setIcon(GraphicsTools::loadColorize(":/images/new/icon_harddrive.svg"));
  connect(Stick20ActionEnableCryptedVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20EnableCryptedVolume()));

  Stick20ActionDisableCryptedVolume = new QAction(tr("&Lock encrypted volume"), main_window);
  Stick20ActionDisableCryptedVolume->setIcon(GraphicsTools::loadColorize(":/images/new/icon_harddrive.svg"));
  connect(Stick20ActionDisableCryptedVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20DisableCryptedVolume()));

  Stick20ActionEnableHiddenVolume = new QAction(tr("&Unlock hidden volume"), main_window);
  Stick20ActionEnableHiddenVolume->setIcon(GraphicsTools::loadColorize(":/images/new/icon_harddrive.svg"));
  connect(Stick20ActionEnableHiddenVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20EnableHiddenVolume()));

  Stick20ActionDisableHiddenVolume = new QAction(tr("&Lock hidden volume"), main_window);
  Stick20ActionDisableHiddenVolume->setIcon(GraphicsTools::loadColorize(":/images/new/icon_harddrive.svg"));
  connect(Stick20ActionDisableHiddenVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20DisableHiddenVolume()));


  Stick20ActionEnableCryptedVolume_tray = new QAction(tr("&Unlock encrypted volume"), main_window);
  Stick20ActionEnableCryptedVolume_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_harddrive.svg", true));
  connect(Stick20ActionEnableCryptedVolume_tray, SIGNAL(triggered()), storageActions,
          SLOT(startStick20EnableCryptedVolume()));

  Stick20ActionDisableCryptedVolume_tray = new QAction(tr("&Lock encrypted volume"), main_window);
  Stick20ActionDisableCryptedVolume_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_harddrive.svg", true));
  connect(Stick20ActionDisableCryptedVolume_tray, SIGNAL(triggered()), storageActions,
          SLOT(startStick20DisableCryptedVolume()));

  Stick20ActionEnableHiddenVolume_tray = new QAction(tr("&Unlock hidden volume"), main_window);
  Stick20ActionEnableHiddenVolume_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_harddrive.svg", true));
  connect(Stick20ActionEnableHiddenVolume_tray, SIGNAL(triggered()), storageActions,
          SLOT(startStick20EnableHiddenVolume()));

  Stick20ActionDisableHiddenVolume_tray = new QAction(tr("&Lock hidden volume"), main_window);
  Stick20ActionDisableHiddenVolume_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_harddrive.svg", true));
  connect(Stick20ActionDisableHiddenVolume_tray, SIGNAL(triggered()), storageActions,
          SLOT(startStick20DisableHiddenVolume()));

  Stick20ActionChangeUserPIN = new QAction(tr("&Change User PIN"), main_window);
  connect(Stick20ActionChangeUserPIN, SIGNAL(triggered()), main_window,
          SLOT(startStick20ActionChangeUserPIN()));

  Stick20ActionChangeAdminPIN = new QAction(tr("&Change Admin PIN"), main_window);
  connect(Stick20ActionChangeAdminPIN, SIGNAL(triggered()), main_window,
          SLOT(startStick20ActionChangeAdminPIN()));

  Stick20ActionChangeUpdatePIN = new QAction(tr("&Change Firmware Password"), main_window);
  connect(Stick20ActionChangeUpdatePIN, SIGNAL(triggered()), main_window,
          SLOT(startStick20ActionChangeUpdatePIN()));

  Stick20ActionEnableFirmwareUpdate = new QAction(tr("&Enable firmware update"), main_window);
  connect(Stick20ActionEnableFirmwareUpdate, SIGNAL(triggered()), storageActions,
          SLOT(startStick20EnableFirmwareUpdate()));

  Stick20ActionExportFirmwareToFile = new QAction(tr("&Export firmware to file"), main_window);
  connect(Stick20ActionExportFirmwareToFile, SIGNAL(triggered()), storageActions,
          SLOT(startStick20ExportFirmwareToFile()));

  QSignalMapper *signalMapper_startStick20DestroyCryptedVolume =
      new QSignalMapper(main_window);

  Stick20ActionDestroyCryptedVolume = new QAction(tr("&Destroy encrypted data"), main_window);
  signalMapper_startStick20DestroyCryptedVolume->setMapping(Stick20ActionDestroyCryptedVolume, 0);
  connect(Stick20ActionDestroyCryptedVolume, SIGNAL(triggered()),
          signalMapper_startStick20DestroyCryptedVolume, SLOT(map()));

  Stick20ActionInitCryptedVolume = new QAction(tr("&Initialize device"), main_window);
  signalMapper_startStick20DestroyCryptedVolume->setMapping(Stick20ActionInitCryptedVolume, 1);
  connect(Stick20ActionInitCryptedVolume, SIGNAL(triggered()),
          signalMapper_startStick20DestroyCryptedVolume, SLOT(map()));

  connect(signalMapper_startStick20DestroyCryptedVolume, SIGNAL(mapped(int)), storageActions,
          SLOT(startStick20DestroyCryptedVolume(int)));

  Stick20ActionFillSDCardWithRandomChars =
      new QAction(tr("&Initialize storage with random data"), main_window);
  connect(Stick20ActionFillSDCardWithRandomChars, SIGNAL(triggered()), storageActions,
          SLOT(startStick20FillSDCardWithRandomChars()));

  Stick20ActionSetReadonlyUncryptedVolume =
      new QAction(tr("&Set unencrypted volume read-only"), main_window);
  connect(Stick20ActionSetReadonlyUncryptedVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20SetReadOnlyUncryptedVolume()));

  Stick20ActionSetReadWriteUncryptedVolume =
      new QAction(tr("&Set unencrypted volume read-write"), main_window);
  connect(Stick20ActionSetReadWriteUncryptedVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20SetReadWriteUncryptedVolume()));

  //do not show until supported
#if 0
  Stick20ActionSetReadonlyEncryptedVolume =
      new QAction(tr("&Set encrypted volume read-only"), main_window);
  connect(Stick20ActionSetReadonlyEncryptedVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20SetReadOnlyEncryptedVolume()));

  Stick20ActionSetReadWriteEncryptedVolume =
      new QAction(tr("&Set encrypted volume read-write"), main_window);
  connect(Stick20ActionSetReadWriteEncryptedVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20SetReadWriteEncryptedVolume()));
#endif

//  Stick20ActionDebugAction = new QAction(tr("&Debug"), main_window);
//  connect(Stick20ActionDebugAction, SIGNAL(triggered()), storageActions, SLOT(startStick20DebugAction()));

  Stick20ActionSetupHiddenVolume = new QAction(tr("&Setup hidden volume"), main_window);
  connect(Stick20ActionSetupHiddenVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20SetupHiddenVolume()));

  Stick20ActionClearNewSDCardFound =
      new QAction(tr("&Disable 'initialize storage with random data' warning"), main_window);
  connect(Stick20ActionClearNewSDCardFound, SIGNAL(triggered()), storageActions,
          SLOT(startStick20ClearNewSdCardFound()));

  Stick20ActionLockStickHardware = new QAction(tr("&Lock stick hardware"), main_window);
  connect(Stick20ActionLockStickHardware, SIGNAL(triggered()), storageActions,
          SLOT(startStick20LockStickHardware()));

  Stick20ActionResetUserPassword = new QAction(tr("&Reset User PIN"), main_window);
  connect(Stick20ActionResetUserPassword, SIGNAL(triggered()), main_window,
          SLOT(startResetUserPassword()));

  LockDeviceAction = new QAction(tr("&Lock Device"), main_window);
  LockDeviceAction->setIcon(GraphicsTools::loadColorize(":/images/new/icon_unsafe.svg"));
  connect(LockDeviceAction, SIGNAL(triggered()), main_window, SLOT(startLockDeviceAction()));

  Stick20ActionUpdateStickStatus = new QAction(tr("Smartcard or SD card are not ready"), main_window);
  connect(Stick20ActionUpdateStickStatus, SIGNAL(triggered()), main_window, SLOT(startAboutDialog()));
}


std::shared_ptr<QThread> thread_tray_populateOTP;

void tray_Worker::doWork() {
  QMutexLocker mutexLocker(&mtx);
  auto passwordSafeUnlocked = libada::i()->isPasswordSafeUnlocked();
  const auto total = TOTP_SLOT_COUNT+HOTP_SLOT_COUNT+
      (passwordSafeUnlocked?PWS_SLOT_COUNT:0)+1;
  int p = 0;
  emit progress(0);

  //populate OTP name cache
  for (int i=0; i < TOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getTOTPSlotName(i);
    emit progress(++p * 100 / total);
  }

  for (int i=0; i<HOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getHOTPSlotName(i);
    emit progress(++p * 100 / total);
  }

  if(passwordSafeUnlocked){
    for (int i=0; i<PWS_SLOT_COUNT; i++){
      if(libada::i()->getPWSSlotStatus(i))
        auto slotName = libada::i()->getPWSSlotName(i);
      emit progress(++p * 100 / total);
    }
  }

  emit progress(100);
  emit resultReady();
}

void Tray::generatePasswordMenu(std::shared_ptr<QMenu> &trayMenu, std::shared_ptr<QMenu> &windowMenu) {

  trayMenuPasswdSubMenu = std::make_shared<QMenu>(tr("Passwords"));
  trayMenuPasswdSubMenu->setIcon(GraphicsTools::loadColorize(":/images/new/icon_passwords.svg"));

  trayMenuPasswdSubMenu_tray = std::make_shared<QMenu>(tr("Passwords"));
  trayMenuPasswdSubMenu_tray->setIcon(GraphicsTools::loadColorize(":/images/new/icon_passwords.svg", true));


  trayMenu->addMenu(trayMenuPasswdSubMenu_tray.get());
  trayMenu->addSeparator();

  windowMenu->addMenu(trayMenuPasswdSubMenu.get());
  windowMenu->addSeparator();

  if (thread_tray_populateOTP!= nullptr){
    destroyThread();
  }
  thread_tray_populateOTP = std::make_shared<QThread>();
  worker = new tray_Worker;
  worker->moveToThread(thread_tray_populateOTP.get());
  connect(thread_tray_populateOTP.get(), &QThread::finished, worker, &QObject::deleteLater);
  connect(thread_tray_populateOTP.get(), SIGNAL(started()), worker, SLOT(doWork()));
  connect(worker, SIGNAL(resultReady()), this, SLOT(populateOTPPasswordMenu()));

  connect(worker, SIGNAL(resultReady()), main_window, SLOT(generateComboBoxEntrys()));
  connect(worker, SIGNAL(progress(int)), this, SLOT(passOTPProgressFurther(int)));
  connect(worker, SIGNAL(progress(int)), this, SLOT(showOTPProgressInTray(int)));

  thread_tray_populateOTP->start();
}

void Tray::destroyThread() {
  if (thread_tray_populateOTP == nullptr) return;
  thread_tray_populateOTP->quit();
  thread_tray_populateOTP->wait();
}

#include "mainwindow.h"
void Tray::populateOTPPasswordMenu() {
  //should not run before worker is done
  QMutexLocker mutexLocker(&worker->mtx);

  if (!trayMenuPasswdSubMenu->actions().empty()) {
    return;
  }


  for (int i=0; i < TOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getTOTPSlotName(i);
    bool slotProgrammed = libada::i()->isTOTPSlotProgrammed(i);
    if (slotProgrammed){
      auto action = trayMenuPasswdSubMenu->addAction(QString::fromStdString(slotName));
      connect(action, SIGNAL(triggered()), mapper_TOTP, SLOT(map()));
      mapper_TOTP->setMapping(action, i) ;
    }
  }

  for (int i=0; i<HOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getHOTPSlotName(i);
    bool slotProgrammed = libada::i()->isHOTPSlotProgrammed(i);
    if (slotProgrammed){
      auto action = trayMenuPasswdSubMenu->addAction(QString::fromStdString(slotName));
      connect(action, SIGNAL(triggered()), mapper_HOTP, SLOT(map()));
      mapper_HOTP->setMapping(action, i) ;
    }
  }

  if (TRUE == libada::i()->isPasswordSafeUnlocked()) {
    for (int i = 0; i < PWS_SLOT_COUNT; i++) {
      if (libada::i()->getPWSSlotStatus(i)) {
        auto slotName = libada::i()->getPWSSlotName(i);
        auto action = trayMenuPasswdSubMenu->addAction(QString::fromStdString(slotName));
        connect(action, SIGNAL(triggered()), mapper_PWS, SLOT(map()));
        mapper_PWS->setMapping(action, i) ;
      }
    }
  }

  if (trayMenuPasswdSubMenu->actions().empty()) {
    trayMenuPasswdSubMenu->setEnabled(false);
    trayMenuPasswdSubMenu->setTitle( trayMenuPasswdSubMenu->title() + " " + tr("(empty)") );
  }

  trayMenuPasswdSubMenu_tray->addActions(trayMenuPasswdSubMenu->actions());

  if (trayMenuPasswdSubMenu_tray->actions().empty()) {
    trayMenuPasswdSubMenu_tray->setEnabled(false);
    trayMenuPasswdSubMenu_tray->setTitle( trayMenuPasswdSubMenu_tray->title() + " " + tr("(empty)") );
  }
}


void Tray::generateMenuForProDevice(std::shared_ptr<QMenu> &trayMenu, std::shared_ptr<QMenu> &windowMenu) {
    generatePasswordMenu(trayMenu, windowMenu);
    trayMenu->addSeparator();
    generateMenuPasswordSafe(trayMenu, windowMenu);

    trayMenuSubConfigure = trayMenu->addMenu(tr("Configure"));
    trayMenuSubConfigure->setIcon(GraphicsTools::loadColorize(":/images/new/icon_settings.svg"));

    if (TRUE == libada::i()->isPasswordSafeAvailable())
      trayMenuSubConfigure->addAction(configureActionStick20);
    else
      trayMenuSubConfigure->addAction(configureAction);

    trayMenuSubConfigure->addSeparator();

    trayMenuSubConfigure->addAction(Stick10ActionChangeUserPIN);
    trayMenuSubConfigure->addAction(Stick10ActionChangeAdminPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeUpdatePIN);
    trayMenuSubConfigure->addAction(Stick20ActionEnableFirmwareUpdate);

    // Enable "reset user PIN" ?
    if (0 == libada::i()->getUserPasswordRetryCount())
    {
      trayMenuSubConfigure->addAction(Stick20ActionResetUserPassword);
    }

    if (ExtendedConfigActive) {
      trayMenuSubConfigure->addSeparator();
      trayMenuSubConfigure->addAction(resetAction);
      trayMenuSubConfigure->addAction(Stick20ActionDestroyCryptedVolume);
    }
    windowMenu->addMenu(trayMenuSubConfigure);
}
using nm = nitrokey::NitrokeyManager;

void Tray::generateMenuForStorageDevice(std::shared_ptr<QMenu> trayMenu, std::shared_ptr<QMenu> windowMenu) {
  int AddSeperator = FALSE;
  {
    nitrokey::proto::stick20::DeviceConfigurationResponsePacket::ResponsePayload status;

    for (int i=0; i < 20; i++){
      try {
        status = nm::instance()->get_status_storage();
        if(status.ActiveSD_CardID_u32 != 0) break;
        auto message = "No active SD card or empty Storage status received, retrying " + std::to_string(i);
        LOG(message, nitrokey::log::Loglevel::DEBUG);
      }
      catch (LongOperationInProgressException &e){
        long_operation_in_progress = true;
        return;
      }
      catch (DeviceCommunicationException &e){
        //TODO add info to tray about the error?
        return;
      }
    }
    long_operation_in_progress = false;

    if (status.ActiveSD_CardID_u32 == 0) // Is Stick 2.0 online (SD + SC
      // accessable?)
    {
      trayMenu->addAction(Stick20ActionUpdateStickStatus);
      LOG("SD card status not active, aborting", nitrokey::log::Loglevel::DEBUG);
      return;
    }

    // Add special entrys
//    if (TRUE == StickNotInitated) {
    if (TRUE == status.StickKeysNotInitiated) {
      trayMenu->addAction(Stick20ActionInitCryptedVolume);
      windowMenu->addAction(Stick20ActionInitCryptedVolume);
      AddSeperator = TRUE;
    }

//    if (FALSE == StickNotInitated && TRUE == SdCardNotErased) {
    if (!status.StickKeysNotInitiated && !status.SDFillWithRandomChars_u8) {
      trayMenu->addAction(Stick20ActionFillSDCardWithRandomChars);
      windowMenu->addAction(Stick20ActionFillSDCardWithRandomChars);
      AddSeperator = TRUE;
    }

    if (TRUE == AddSeperator){
      trayMenu->addSeparator();
      windowMenu->addSeparator();
    }

    generatePasswordMenu(trayMenu, windowMenu);
    trayMenu->addSeparator();

    if (!status.StickKeysNotInitiated) {
      // Setup entrys for password safe
        generateMenuPasswordSafe(trayMenu, windowMenu);
    }

    if (status.SDFillWithRandomChars_u8) { //filled randomly
      if (!status.VolumeActiceFlag_st.encrypted){
        trayMenu->addAction(Stick20ActionEnableCryptedVolume_tray);
        windowMenu->addAction(Stick20ActionEnableCryptedVolume);
      }
      else{
        trayMenu->addAction(Stick20ActionDisableCryptedVolume_tray);
        windowMenu->addAction(Stick20ActionDisableCryptedVolume);
      }

      if (!status.VolumeActiceFlag_st.hidden){
        trayMenu->addAction(Stick20ActionEnableHiddenVolume_tray);
        windowMenu->addAction(Stick20ActionEnableHiddenVolume);
      }
      else{
        trayMenu->addAction(Stick20ActionDisableHiddenVolume_tray);
        windowMenu->addAction(Stick20ActionDisableHiddenVolume);
      }
    }

    //FIXME run in separate thread
    const auto PasswordSafeEnabled = libada::i()->isPasswordSafeUnlocked();
    if (false != (status.VolumeActiceFlag_st.hidden || status.VolumeActiceFlag_st.encrypted || PasswordSafeEnabled)){
      trayMenu->addAction(LockDeviceAction);
      windowMenu->addAction(LockDeviceAction);
    }

    trayMenuSubConfigure = trayMenu->addMenu(tr("Configure"));
    trayMenuSubConfigure->setIcon(GraphicsTools::loadColorize(":/images/new/icon_settings.svg"));
    trayMenuSubConfigure->addAction(configureActionStick20);
    trayMenuSubConfigure->addSeparator();

    // Pin actions
    trayMenuSubConfigure->addAction(Stick20ActionChangeUserPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeAdminPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeUpdatePIN);
    trayMenuSubConfigure->addSeparator();

    // Storage actions
    auto read_write_active = status.ReadWriteFlagUncryptedVolume_u8 == 0;
    if (read_write_active)
      // Set readonly active
      trayMenuSubConfigure->addAction(Stick20ActionSetReadonlyUncryptedVolume);
    else
      // Set RW active
      trayMenuSubConfigure->addAction(Stick20ActionSetReadWriteUncryptedVolume);

#if 0
    auto read_write_active_encrypted = status.ReadWriteFlagCryptedVolume_u8 == 0;
    trayMenuSubConfigure->addAction(
        read_write_active_encrypted ?
        Stick20ActionSetReadonlyEncryptedVolume : Stick20ActionSetReadWriteEncryptedVolume
    );
#endif

//    if (FALSE == SdCardNotErased)
    if (status.SDFillWithRandomChars_u8)
      trayMenuSubConfigure->addAction(Stick20ActionSetupHiddenVolume);

    trayMenuSubConfigure->addAction(Stick20ActionDestroyCryptedVolume);
    trayMenuSubConfigure->addSeparator();

    // Other actions
//    if (TRUE == LockHardware) //FIXME
//      trayMenuSubConfigure->addAction(Stick20ActionLockStickHardware);


    trayMenuSubConfigure->addAction(Stick20ActionEnableFirmwareUpdate);
    trayMenuSubConfigure->addAction(Stick20ActionExportFirmwareToFile);


    if (TRUE == ExtendedConfigActive) {
      trayMenuSubConfigure->addSeparator();
      trayMenuSubSpecialConfigure = trayMenuSubConfigure->addMenu(tr("Special Configure"));
      trayMenuSubSpecialConfigure->addAction(Stick20ActionFillSDCardWithRandomChars);

      if (status.NewSDCardFound_u8 && !status.SDFillWithRandomChars_u8)
        trayMenuSubSpecialConfigure->addAction(Stick20ActionClearNewSDCardFound);

      trayMenuSubSpecialConfigure->addAction(resetAction);
    }
    windowMenu->addMenu(trayMenuSubConfigure);

    // Enable "reset user PIN" ?
    if (0 == libada::i()->getUserPasswordRetryCount()) {
      trayMenu->addSeparator();
      trayMenu->addAction(Stick20ActionResetUserPassword);
      windowMenu->addAction(Stick20ActionResetUserPassword);
    }

//    // Add debug window ?
//    if (TRUE == DebugWindowActive) {
//      trayMenu->addSeparator();
//      trayMenu->addAction(Stick20ActionDebugAction);
//    }

  }
}

int Tray::UpdateDynamicMenuEntrys(void) {
  generateMenu(false);
  return (TRUE);
}

void Tray::generateMenuPasswordSafe(std::shared_ptr<QMenu> &trayMenu, std::shared_ptr<QMenu> &windowMenu) {
  auto passwordSafeUnlocked = libada::i()->isPasswordSafeUnlocked();
  if (!passwordSafeUnlocked) {
      trayMenu->addAction(UnlockPasswordSafeAction_tray);
      windowMenu->addAction(UnlockPasswordSafeAction);

      auto passwordSafeAvailable = libada::i()->isPasswordSafeAvailable();
      UnlockPasswordSafeAction->setEnabled(passwordSafeAvailable);
      UnlockPasswordSafeAction_tray->setEnabled(passwordSafeAvailable);
  } else {
    trayMenu->addAction(LockDeviceAction);
    windowMenu->addAction(LockDeviceAction);
  }
}

void Tray::regenerateMenu() {
  generateMenu(false);
}

void Tray::passOTPProgressFurther(int i) {
  emit progress(i);
}

void Tray::showOTPProgressInTray(int i) {
  static const QString &s = trayMenuPasswdSubMenu->title();
  if (i!=100)
    trayMenuPasswdSubMenu->setTitle(s +" ("+ QString::number(i) + "%)");
  else
    trayMenuPasswdSubMenu->setTitle(s);
}

void Tray::updateOperationInProgressBar(int p) {
  auto te = QString(tr("Long operation in progress: %1%")).arg(p);
  static auto a = std::make_shared<QAction>("", nullptr);
  connect(a.get(), SIGNAL(triggered()), main_window, SLOT(show_progress_window()));
  if (trayMenu == nullptr) {
    generateMenu(true);
  }
  static bool initialized = false;
  if (!initialized){
    this->showTrayMessage(te);
    trayMenu->addAction(a.get());
    initialized = true;
  }
  a->setText(te);
  trayIcon->setContextMenu(trayMenu.get());
}

void Tray::setAdmin_mode(bool _admin_mode) {
  ExtendedConfigActive = _admin_mode;
}

void Tray::setFile_menu(QMenuBar *file_menu) {
  Tray::file_menu = file_menu;
  generateMenu(true);
  if (windowMenu!= nullptr)
    file_menu->addMenu(windowMenu.get());
}

