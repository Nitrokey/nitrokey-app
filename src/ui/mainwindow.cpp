/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *                      Parts Rudolf Boeddeker  Date: 2013-08-13
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
#include "libnitrokey/include/NitrokeyManager.h"

#include "mainwindow.h"
#include "aboutdialog.h"
#include "base32.h"
#include "pindialog.h"
#include "sleep.h"
#include "stick20debugdialog.h"
#include "string.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <string.h>

#include "mcvs-wrapper.h"
#include "nitrokey-applet.h"
#include "securitydialog.h"
#include "stick20-response-task.h"
#include "stick20changepassworddialog.h"
#include "stick20hiddenvolumedialog.h"
#include "stick20lockfirmwaredialog.h"
#include "stick20responsedialog.h"
#include "stick20updatedialog.h"
#include "libada.h"

#include <QDateTime>
#include <QDialog>
#include <QMenu>
#include <QThread>
#include <QTimer>
#include <QtWidgets>

#ifdef Q_OS_LINUX
#include <libintl.h>
#include <locale.h>
#define _(String) gettext(String)
#include "systemutils.h"
#include <errno.h>     // for unmounting on linux
#include <sys/mount.h> // for unmounting on linux
#endif                 // Q_OS_LINUX

#include <stdio.h> //for fflush to sync on all OSes including Windows
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <unistd.h> //for sync syscall
#endif              // Q_OS_LINUX || Q_OS_MAC

#include <QString>

/*******************************************************************************
 External declarations
*******************************************************************************/

extern "C" void DebugAppendTextGui(const char *Text);

extern "C" void DebugInitDebugging(void);

/*******************************************************************************
 Local defines
*******************************************************************************/

#include <algorithm>

class OwnSleep : public QThread {
public:
  static void usleep(unsigned long usecs) { QThread::usleep(usecs); }
  static void msleep(unsigned long msecs) { QThread::msleep(msecs); }
  static void sleep(unsigned long secs) { QThread::sleep(secs); }
};

void unmountEncryptedVolumes() {
// TODO check will this work also on Mac
#if defined(Q_OS_LINUX)
  std::string endev = systemutils::getEncryptedDevice();
  if (endev.size() < 1)
    return;
  std::string mntdir = systemutils::getMntPoint(endev);
//  if (DebugingActive == TRUE)
    qDebug() << "Unmounting " << mntdir.c_str();
  // TODO polling with MNT_EXPIRE? test which will suit better
  // int err = umount2("/dev/nitrospace", MNT_DETACH);
  int err = umount(mntdir.c_str());
  if (err != 0) {
//    if (DebugingActive == TRUE)
      qDebug() << "Unmount error: " << strerror(errno);
  }
#endif // Q_OS_LINUX
}

void local_sync() {
  // TODO TEST unmount during/after big data transfer
  fflush(NULL); // for windows, not necessarly needed or working
#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
  sync();
#endif // Q_OS_LINUX || Q_OS_MAC
  // manual says sync waits until it's done, but they
  // are not guaranteeing will this save data integrity anyway,
  // additional sleep should help
  OwnSleep::sleep(2);
  // unmount does sync on its own additionally (if successful)
  unmountEncryptedVolumes();
}

#define LOCAL_PASSWORD_SIZE 40

void MainWindow::overwrite_string(QString &str) { std::fill(str.begin(), str.end(), '*'); }

void MainWindow::showTrayMessage(QString message) {
    showTrayMessage("Nitrokey App", message, INFORMATION, 2000);
}

void MainWindow::showTrayMessage(const QString &title, const QString &msg,
                                 enum trayMessageType type, int timeout) {
    if (TRUE == trayIcon->supportsMessages()) {
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

/*
 * Create the tray menu.
 * In Unity we create an AppIndicator
 * In all other systems we use Qt's tray
 */
void MainWindow::createIndicator() {
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/images/CS_icon.png"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
            SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    trayIcon->show();

  // Initial message
  if (TRUE == DebugWindowActive)
    showTrayMessage("Nitrokey App", tr("Active (debug mode)"), INFORMATION, TRAY_MSG_TIMEOUT);
  else
    showTrayMessage("Nitrokey App", tr("Active"), INFORMATION, TRAY_MSG_TIMEOUT);
}

void MainWindow::InitState() {
  HOTP_SlotCount = HOTP_SLOT_COUNT;
  TOTP_SlotCount = TOTP_SLOT_COUNT;

  trayMenu = NULL;
  Stick20ScSdCardOnline = FALSE;
  CryptedVolumeActive = FALSE;
  HiddenVolumeActive = FALSE;
  NormalVolumeRWActive = FALSE;
  HiddenVolumeAccessable = FALSE;
  StickNotInitated = FALSE;
  SdCardNotErased = FALSE;
  MatrixInputActive = FALSE;
  LockHardware = FALSE;
  PasswordSafeEnabled = FALSE;

  SdCardNotErased_DontAsk = FALSE;
  StickNotInitated_DontAsk = FALSE;

  PWS_Access = FALSE;
  PWS_CreatePWSize = 12;
}

MainWindow::MainWindow(StartUpParameter_tst *StartupInfo_st, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), trayMenuPasswdSubMenu(NULL),
      otpInClipboard("not empty"), secretInClipboard("not empty"), PWSInClipboard("not empty") {
#ifdef Q_OS_LINUX
  setlocale(LC_ALL, "");
  bindtextdomain("nitrokey-app", "/usr/share/locale");
  textdomain("nitrokey-app");
#endif
  lastUserAuthenticateTime = lastClipboardTime = QDateTime::currentDateTime().toTime_t();
  int ret;

  QMetaObject::Connection ret_connection;

  InitState();
  clipboard = QApplication::clipboard();
  ExtendedConfigActive = StartupInfo_st->ExtendedConfigActive;

  if (0 != StartupInfo_st->LockHardware)
    LockHardware = TRUE;

  nitrokey::NitrokeyManager::instance()->connect();


//  switch (StartupInfo_st->FlagDebug) {
//  case DEBUG_STATUS_LOCAL_DEBUG:
//    DebugWindowActive = TRUE;
//    DebugingActive = TRUE;
//    DebugingStick20PoolingActive = FALSE;
//    break;
//
//  case DEBUG_STATUS_DEBUG_ALL:
//    DebugWindowActive = TRUE;
//    DebugingActive = TRUE;
//    DebugingStick20PoolingActive = TRUE;
//    break;
//
//  case DEBUG_STATUS_NO_DEBUGGING:
//  default:
//    DebugWindowActive = FALSE;
//    DebugingActive = FALSE;
//    DebugingStick20PoolingActive = FALSE;
//    break;
//  }

  ui->setupUi(this);
  ui->tabWidget->setCurrentIndex(0); // Set first tab active
  ui->PWS_ButtonCreatePW->setText(QString(tr("Generate random password ")));
  ui->statusBar->showMessage(tr("Nitrokey disconnected"));
//  cryptostick = new Device(VID_STICK_OTP, PID_STICK_OTP, VID_STICK_20, PID_STICK_20,
//                           VID_STICK_20_UPDATE_MODE, PID_STICK_20_UPDATE_MODE);

  // Check for comamd line execution after init "nitrokey"
  if (0 != StartupInfo_st->Cmd) {
//    initDebugging();
    ret = ExecStickCmd(StartupInfo_st->CmdLine);
    exit(ret);
  }

  set_initial_time = false;
  QTimer *timer = new QTimer(this);

  ret_connection = connect(timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
  timer->start(2000);

  QTimer *Clipboard_ValidTimer = new QTimer(this);

  // Start timer for Clipboard delete check
  connect(Clipboard_ValidTimer, SIGNAL(timeout()), this, SLOT(checkClipboard_Valid()));
  Clipboard_ValidTimer->start(2000);

  QTimer *Password_ValidTimer = new QTimer(this);

  // Start timer for Password check
  connect(Password_ValidTimer, SIGNAL(timeout()), this, SLOT(checkPasswordTime_Valid()));
  Password_ValidTimer->start(2000);

  createIndicator();

  initActionsForStick10();
  initActionsForStick20();
  initCommonActions();

  connect(ui->secretEdit, SIGNAL(textEdited(QString)), this, SLOT(checkTextEdited()));

  // Init debug text
//  initDebugging();

  ui->deleteUserPasswordCheckBox->setEnabled(false);
  ui->deleteUserPasswordCheckBox->setChecked(false);

  //TODO
//    translateDeviceStatusToUserMessage(cryptostick->getStatus());
  generateMenu();

  ui->PWS_EditPassword->setValidator(
      new utf8FieldLengthValidator(PWS_PASSWORD_LENGTH, ui->PWS_EditPassword));
  ui->PWS_EditLoginName->setValidator(
      new utf8FieldLengthValidator(PWS_LOGINNAME_LENGTH, ui->PWS_EditLoginName));
  ui->PWS_EditSlotName->setValidator(
      new utf8FieldLengthValidator(PWS_SLOTNAME_LENGTH, ui->PWS_EditSlotName));
}

#define MAX_CONNECT_WAIT_TIME_IN_SEC 10

int MainWindow::ExecStickCmd(const char *Cmdline_) {
  return (0);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason) {

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

void MainWindow::translateDeviceStatusToUserMessage(const int getStatus){
    switch (getStatus) {
        case 1:
            //regained connection
            showTrayMessage(
                tr("Regained connection to the device.")
            );
        break;
        case -10:
            // problems with communication, received CRC other than expected, try to reinitialize
            showTrayMessage(
                    tr("Detected some communication problems with the device. Reinitializing.")
            );
            break;
        case -11:
            // fatal error, cannot resume communication, ask user for reinsertion
            csApplet()->warningBox(
                tr("Device lock detected, please remove and insert the device again.\nIf problem will occur again please: \n1. Close the application\n2. Reinsert the device\n3. Wait 30 seconds and start application")
            );
            break;
        default:
            break;
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
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

int MainWindow::AnalyseProductionInfos() {
return 0;
}

void MainWindow::checkConnection() {
//    if (!check_connection_mutex.tryLock(500))
//        return;
//  check_connection_mutex.unlock();

  QMutexLocker locker(&check_connection_mutex);

  //move to constructor
  HOTP_SlotCount = HOTP_SLOT_COUNT;
  TOTP_SlotCount = TOTP_SLOT_COUNT;


  static int DeviceOffline = TRUE;
  int ret = 0;
  currentTime = QDateTime::currentDateTime().toTime_t();

  int result = libada::i()->isDeviceConnected()? 0 : -1;


  if (result == 0) { //connected
    if (false == libada::i()->isStorageDeviceConnected()) {
      ui->statusBar->showMessage(tr("Nitrokey Pro connected"));
//        translateDeviceStatusToUserMessage(cryptostick->getStatus()); //TODO
    } else
      ui->statusBar->showMessage(tr("Nitrokey Storage connected"));

    DeviceOffline = FALSE;

  } else if (result == -1) { //disconnected
    ui->statusBar->showMessage(tr("Nitrokey disconnected"));
    if (FALSE == DeviceOffline) // To avoid the continuous reseting of
                                // the menu
    {
      generateMenu();
      DeviceOffline = TRUE;
      showTrayMessage(tr("Nitrokey disconnected"), "", INFORMATION, TRAY_MSG_TIMEOUT);
    }

  } else if (result == 1) { // recreate the settings and menus
    if (false == libada::i()->isStorageDeviceConnected()) {
      ui->statusBar->showMessage(tr("Nitrokey connected"));
      showTrayMessage(tr("Nitrokey connected"), "Nitrokey Pro", INFORMATION, TRAY_MSG_TIMEOUT);
//      initialTimeReset(ret); // TODO make call just before getting TOTP instead of every connection
//        translateDeviceStatusToUserMessage(cryptostick->getStatus()); //TODO
    } else {
      ui->statusBar->showMessage(tr("Nitrokey Storage connected"));
      showTrayMessage(tr("Nitrokey connected"), "Nitrokey Storage", INFORMATION, TRAY_MSG_TIMEOUT);
    }
    generateMenu();
  }

//  if (TRUE == Stick20_ConfigurationChanged && libada::i()->isStorageDeviceConnected()) {
//
//    UpdateDynamicMenuEntrys();
//
//    if (TRUE == StickNotInitated) {
//      if (FALSE == StickNotInitated_DontAsk)
//          csApplet()->warningBox(tr("Warning: Encrypted volume is not secure,\nSelect \"Initialize "
//                                            "device\" option from context menu."));
//    }
//    if (FALSE == StickNotInitated && TRUE == SdCardNotErased) {
//      if (FALSE == SdCardNotErased_DontAsk)
//          csApplet()->warningBox(tr("Warning: Encrypted volume is not secure,\nSelect \"Initialize "
//                                            "storage with random data\""));
//    }
//  }
}

void MainWindow::initialTimeReset(int ret) {
  //TODO do it only on TOTP code access

//  if (set_initial_time == FALSE) {
//        ret = cryptostick->setTime(TOTP_CHECK_TIME);
//        set_initial_time = TRUE;
//      } else {
//        ret = 0;
//      }
//
//  bool answer;
//
//  if (ret == -2) {
//        answer = csApplet()->detailedYesOrNoBox(tr("Time is out-of-sync"),
//                                                tr("WARNING!\n\nThe time of your computer and Nitrokey are out of "
//                                                           "sync. Your computer may be configured with a wrong time or "
//                                                           "your Nitrokey may have been attacked. If an attacker or "
//                                                           "malware could have used your Nitrokey you should reset the "
//                                                           "secrets of your configured One Time Passwords. If your "
//                                                           "computer's time is wrong, please configure it correctly and "
//                                                           "reset the time of your Nitrokey.\n\nReset Nitrokey's time?"),
//                                                false);
//        if (answer) {
//          resetTime();
//          QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//          Sleep::msleep(1000);
//          QGuiApplication::restoreOverrideCursor();
//          generateAllConfigs();
//
//            csApplet()->messageBox(tr("Time reset!"));
//        }
//      }
}

void MainWindow::startTimer() {}

MainWindow::~MainWindow() {
  checkClipboard_Valid(true);
  delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
  this->hide();
  event->ignore();
}

void MainWindow::getSlotNames() {}

void MainWindow::generateComboBoxEntrys() {
  int i;

  ui->slotComboBox->clear();

  for (i = 0; i < TOTP_SlotCount; i++) {
    if (libada::i()->getTOTPSlotName(i).empty())
      ui->slotComboBox->addItem(QString(tr("TOTP slot ")).append(QString::number(i + 1, 10)));
    else
      ui->slotComboBox->addItem(QString(tr("TOTP slot "))
                                    .append(QString::number(i + 1, 10))
                                    .append(" [")
                                    .append(QString::fromStdString(libada::i()->getTOTPSlotName(i)))
                                    .append("]"));
  }

  ui->slotComboBox->insertSeparator(TOTP_SlotCount + 1);

  for (i = 0; i < HOTP_SlotCount; i++) {
    if (libada::i()->getHOTPSlotName(i).empty())
      ui->slotComboBox->addItem(QString(tr("HOTP slot ")).append(QString::number(i + 1, 10)));
    else
      ui->slotComboBox->addItem(QString(tr("HOTP slot "))
                                    .append(QString::number(i + 1, 10))
                                    .append(" [")
                                    .append(QString::fromStdString(libada::i()->getHOTPSlotName(i)))
                                    .append("]"));
  }

  ui->slotComboBox->setCurrentIndex(0);
}

void MainWindow::generateMenu() {
  {
    if (NULL == trayMenu)
      trayMenu = new QMenu();
    else
      trayMenu->clear(); // Clear old menu

    // Setup the new menu
    if (libada::i()->isDeviceConnected() == false) {
      trayMenu->addAction(tr("Nitrokey not connected"));
    } else {
      if (false == libada::i()->isStorageDeviceConnected()) // Nitrokey Pro connected
        generateMenuForProDevice();
      else {
        // Nitrokey Storage is connected
        generateMenuForStorageDevice();
      }
    }

    // Add debug window ?
    if (TRUE == DebugWindowActive)
      trayMenu->addAction(DebugAction);

    trayMenu->addSeparator();

    // About entry
    trayMenu->addAction(ActionAboutDialog);

    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu);
  }
  generateComboBoxEntrys();
}

void MainWindow::initActionsForStick10() {
  UnlockPasswordSafeAction = new QAction(tr("Unlock password safe"), this);
  UnlockPasswordSafeAction->setIcon(QIcon(":/images/safe.png"));
  connect(UnlockPasswordSafeAction, SIGNAL(triggered()), this, SLOT(PWS_Clicked_EnablePWSAccess()));

  configureAction = new QAction(tr("&OTP"), this);
  connect(configureAction, SIGNAL(triggered()), this, SLOT(startConfiguration()));

  resetAction = new QAction(tr("&Factory reset"), this);
  connect(resetAction, SIGNAL(triggered()), this, SLOT(factoryReset()));

  Stick10ActionChangeUserPIN = new QAction(tr("&Change User PIN"), this);
  connect(Stick10ActionChangeUserPIN, SIGNAL(triggered()), this,
          SLOT(startStick10ActionChangeUserPIN()));

  Stick10ActionChangeAdminPIN = new QAction(tr("&Change Admin PIN"), this);
  connect(Stick10ActionChangeAdminPIN, SIGNAL(triggered()), this,
          SLOT(startStick10ActionChangeAdminPIN()));
}

void MainWindow::initCommonActions() {
  DebugAction = new QAction(tr("&Debug"), this);
  connect(DebugAction, SIGNAL(triggered()), this, SLOT(startStickDebug()));

  quitAction = new QAction(tr("&Quit"), this);
  quitAction->setIcon(QIcon(":/images/quit.png"));
  connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

  ActionAboutDialog = new QAction(tr("&About Nitrokey"), this);

  ActionAboutDialog->setIcon(QIcon(":/images/about.png"));
  connect(ActionAboutDialog, SIGNAL(triggered()), this, SLOT(startAboutDialog()));
}

void MainWindow::initActionsForStick20() {
  configureActionStick20 = new QAction(tr("&OTP and Password safe"), this);
  connect(configureActionStick20, SIGNAL(triggered()), this, SLOT(startConfiguration()));

  Stick20ActionEnableCryptedVolume = new QAction(tr("&Unlock encrypted volume"), this);
  Stick20ActionEnableCryptedVolume->setIcon(QIcon(":/images/harddrive.png"));
  connect(Stick20ActionEnableCryptedVolume, SIGNAL(triggered()), this,
          SLOT(startStick20EnableCryptedVolume()));

  Stick20ActionDisableCryptedVolume = new QAction(tr("&Lock encrypted volume"), this);
  Stick20ActionDisableCryptedVolume->setIcon(QIcon(":/images/harddrive.png"));
  connect(Stick20ActionDisableCryptedVolume, SIGNAL(triggered()), this,
          SLOT(startStick20DisableCryptedVolume()));

  Stick20ActionEnableHiddenVolume = new QAction(tr("&Unlock hidden volume"), this);
  Stick20ActionEnableHiddenVolume->setIcon(QIcon(":/images/harddrive.png"));
  connect(Stick20ActionEnableHiddenVolume, SIGNAL(triggered()), this,
          SLOT(startStick20EnableHiddenVolume()));

  Stick20ActionDisableHiddenVolume = new QAction(tr("&Lock hidden volume"), this);
  connect(Stick20ActionDisableHiddenVolume, SIGNAL(triggered()), this,
          SLOT(startStick20DisableHiddenVolume()));

  Stick20ActionChangeUserPIN = new QAction(tr("&Change User PIN"), this);
  connect(Stick20ActionChangeUserPIN, SIGNAL(triggered()), this,
          SLOT(startStick20ActionChangeUserPIN()));

  Stick20ActionChangeAdminPIN = new QAction(tr("&Change Admin PIN"), this);
  connect(Stick20ActionChangeAdminPIN, SIGNAL(triggered()), this,
          SLOT(startStick20ActionChangeAdminPIN()));

  Stick20ActionChangeUpdatePIN = new QAction(tr("&Change Firmware Password"), this);
  connect(Stick20ActionChangeUpdatePIN, SIGNAL(triggered()), this,
          SLOT(startStick20ActionChangeUpdatePIN()));

  Stick20ActionEnableFirmwareUpdate = new QAction(tr("&Enable firmware update"), this);
  connect(Stick20ActionEnableFirmwareUpdate, SIGNAL(triggered()), this,
          SLOT(startStick20EnableFirmwareUpdate()));

  Stick20ActionExportFirmwareToFile = new QAction(tr("&Export firmware to file"), this);
  connect(Stick20ActionExportFirmwareToFile, SIGNAL(triggered()), this,
          SLOT(startStick20ExportFirmwareToFile()));

  QSignalMapper *signalMapper_startStick20DestroyCryptedVolume =
      new QSignalMapper(this); // FIXME memory leak

  Stick20ActionDestroyCryptedVolume = new QAction(tr("&Destroy encrypted data"), this);
  signalMapper_startStick20DestroyCryptedVolume->setMapping(Stick20ActionDestroyCryptedVolume, 0);
  connect(Stick20ActionDestroyCryptedVolume, SIGNAL(triggered()),
          signalMapper_startStick20DestroyCryptedVolume, SLOT(map()));

  Stick20ActionInitCryptedVolume = new QAction(tr("&Initialize device"), this);
  signalMapper_startStick20DestroyCryptedVolume->setMapping(Stick20ActionInitCryptedVolume, 1);
  connect(Stick20ActionInitCryptedVolume, SIGNAL(triggered()),
          signalMapper_startStick20DestroyCryptedVolume, SLOT(map()));

  connect(signalMapper_startStick20DestroyCryptedVolume, SIGNAL(mapped(int)), this,
          SLOT(startStick20DestroyCryptedVolume(int)));

  Stick20ActionFillSDCardWithRandomChars =
      new QAction(tr("&Initialize storage with random data"), this);
  connect(Stick20ActionFillSDCardWithRandomChars, SIGNAL(triggered()), this,
          SLOT(startStick20FillSDCardWithRandomChars()));

  Stick20ActionSetReadonlyUncryptedVolume =
      new QAction(tr("&Set unencrypted volume read-only"), this);
  connect(Stick20ActionSetReadonlyUncryptedVolume, SIGNAL(triggered()), this,
          SLOT(startStick20SetReadOnlyUncryptedVolume()));

  Stick20ActionSetReadWriteUncryptedVolume =
      new QAction(tr("&Set unencrypted volume read-write"), this);
  connect(Stick20ActionSetReadWriteUncryptedVolume, SIGNAL(triggered()), this,
          SLOT(startStick20SetReadWriteUncryptedVolume()));

  Stick20ActionDebugAction = new QAction(tr("&Debug"), this);
  connect(Stick20ActionDebugAction, SIGNAL(triggered()), this, SLOT(startStick20DebugAction()));

  Stick20ActionSetupHiddenVolume = new QAction(tr("&Setup hidden volume"), this);
  connect(Stick20ActionSetupHiddenVolume, SIGNAL(triggered()), this,
          SLOT(startStick20SetupHiddenVolume()));

  Stick20ActionClearNewSDCardFound =
      new QAction(tr("&Disable 'initialize storage with random data' warning"), this);
  connect(Stick20ActionClearNewSDCardFound, SIGNAL(triggered()), this,
          SLOT(startStick20ClearNewSdCardFound()));

  Stick20ActionSetupPasswordMatrix = new QAction(tr("&Setup password matrix"), this);
  connect(Stick20ActionSetupPasswordMatrix, SIGNAL(triggered()), this,
          SLOT(startStick20SetupPasswordMatrix()));

  Stick20ActionLockStickHardware = new QAction(tr("&Lock stick hardware"), this);
  connect(Stick20ActionLockStickHardware, SIGNAL(triggered()), this,
          SLOT(startStick20LockStickHardware()));

  Stick20ActionResetUserPassword = new QAction(tr("&Reset User PIN"), this);
  connect(Stick20ActionResetUserPassword, SIGNAL(triggered()), this,
          SLOT(startResetUserPassword()));

  LockDeviceAction = new QAction(tr("&Lock Device"), this);
  connect(LockDeviceAction, SIGNAL(triggered()), this, SLOT(startLockDeviceAction()));

  Stick20ActionUpdateStickStatus = new QAction(tr("Smartcard or SD card are not ready"), this);
  connect(Stick20ActionUpdateStickStatus, SIGNAL(triggered()), this, SLOT(startAboutDialog()));
}

void MainWindow::generatePasswordMenu() {
  {
    if (trayMenuPasswdSubMenu != NULL) {
      delete trayMenuPasswdSubMenu;
    }
    trayMenuPasswdSubMenu = new QMenu(tr("Passwords")); //TODO make shared pointer

    for (int i=0; i<TOTP_SLOT_COUNT; i++){
      if (libada::i()->isTOTPSlotProgrammed(i)){
        trayMenuPasswdSubMenu->addAction(QString::fromStdString(libada::i()->getTOTPSlotName(i)),
        this, [=](){getTOTPDialog(i);} );
      }
    }

    for (int i=0; i<HOTP_SLOT_COUNT; i++){
      if (libada::i()->isTOTPSlotProgrammed(i)){
        trayMenuPasswdSubMenu->addAction(QString::fromStdString(libada::i()->getHOTPSlotName(i)),
        this, [=](){getHOTPDialog(i);} );
      }
    }

    if (TRUE == libada::i()->isPasswordSafeUnlocked()) {
      for (int i = 0; i < PWS_SLOT_COUNT; i++) {
        if (libada::i()->getPWSSlotStatus(i)) {
          trayMenuPasswdSubMenu->addAction(QString::fromStdString(libada::i()->getPWSSlotName(i)),
                                           this, [=]() { PWS_ExceClickedSlot(i); });
        }
      }
    }

     if (!trayMenuPasswdSubMenu->actions().empty()) {
      trayMenu->addMenu(trayMenuPasswdSubMenu);
      trayMenu->addSeparator();
    }
  }
}

void MainWindow::generateMenuForProDevice() {
  {
    generatePasswordMenu();
    trayMenu->addSeparator();
    generateMenuPasswordSafe();

    trayMenuSubConfigure = trayMenu->addMenu(tr("Configure"));
    trayMenuSubConfigure->setIcon(QIcon(":/images/settings.png"));

    if (TRUE == libada::i()->isPasswordSafeAvailable())
      trayMenuSubConfigure->addAction(configureActionStick20);
    else
      trayMenuSubConfigure->addAction(configureAction);

    trayMenuSubConfigure->addSeparator();

    trayMenuSubConfigure->addAction(Stick10ActionChangeUserPIN);
    trayMenuSubConfigure->addAction(Stick10ActionChangeAdminPIN);

    // Enable "reset user PIN" ?
    if (0 == libada::i()->getUserPasswordRetryCount())
    {
      trayMenuSubConfigure->addAction(Stick20ActionResetUserPassword);
    }

    if (ExtendedConfigActive) {
      trayMenuSubConfigure->addSeparator();
      trayMenuSubConfigure->addAction(resetAction);
    }
  }
}

void MainWindow::generateMenuForStorageDevice() {
  int AddSeperator = FALSE;
  {

    if (FALSE == Stick20ScSdCardOnline) // Is Stick 2.0 online (SD + SC
                                        // accessable?)
    {
      trayMenu->addAction(Stick20ActionUpdateStickStatus);
      return;
    }

    // Add special entrys
    if (TRUE == StickNotInitated) {
      trayMenu->addAction(Stick20ActionInitCryptedVolume);
      AddSeperator = TRUE;
    }

    if (FALSE == StickNotInitated && TRUE == SdCardNotErased) {
      trayMenu->addAction(Stick20ActionFillSDCardWithRandomChars);
      AddSeperator = TRUE;
    }

    if (TRUE == AddSeperator)
      trayMenu->addSeparator();

    generatePasswordMenu();
    trayMenu->addSeparator();

    if (FALSE == StickNotInitated) {
      // Enable tab for password safe for stick 2
      if (-1 == ui->tabWidget->indexOf(ui->tab_3)) {
        ui->tabWidget->addTab(ui->tab_3, tr("Password Safe"));
      }

      // Setup entrys for password safe
      generateMenuPasswordSafe();
    }

    if (FALSE == SdCardNotErased) {
      if (FALSE == CryptedVolumeActive)
        trayMenu->addAction(Stick20ActionEnableCryptedVolume);
      else
        trayMenu->addAction(Stick20ActionDisableCryptedVolume);

      if (FALSE == HiddenVolumeActive)
        trayMenu->addAction(Stick20ActionEnableHiddenVolume);
      else
        trayMenu->addAction(Stick20ActionDisableHiddenVolume);
    }

    if (FALSE != (HiddenVolumeActive || CryptedVolumeActive || PasswordSafeEnabled))
      trayMenu->addAction(LockDeviceAction);

    trayMenuSubConfigure = trayMenu->addMenu(tr("Configure"));
    trayMenuSubConfigure->setIcon(QIcon(":/images/settings.png"));
    trayMenuSubConfigure->addAction(configureActionStick20);
    trayMenuSubConfigure->addSeparator();

    // Pin actions
    trayMenuSubConfigure->addAction(Stick20ActionChangeUserPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeAdminPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeUpdatePIN);
    if (TRUE == MatrixInputActive)
      trayMenuSubConfigure->addAction(Stick20ActionSetupPasswordMatrix);
    trayMenuSubConfigure->addSeparator();

    // Storage actions
    if (FALSE == NormalVolumeRWActive)
      trayMenuSubConfigure->addAction(Stick20ActionSetReadonlyUncryptedVolume); // Set
    // RW
    // active
    else
      trayMenuSubConfigure->addAction(Stick20ActionSetReadWriteUncryptedVolume); // Set
                                                                                 // readonly
                                                                                 // active

    if (FALSE == SdCardNotErased)
      trayMenuSubConfigure->addAction(Stick20ActionSetupHiddenVolume);

    trayMenuSubConfigure->addAction(Stick20ActionDestroyCryptedVolume);
    trayMenuSubConfigure->addSeparator();

    // Other actions
    if (TRUE == LockHardware)
      trayMenuSubConfigure->addAction(Stick20ActionLockStickHardware);

    if (TRUE == HiddenVolumeAccessable) {
    }

    trayMenuSubConfigure->addAction(Stick20ActionEnableFirmwareUpdate);
    trayMenuSubConfigure->addAction(Stick20ActionExportFirmwareToFile);

    trayMenuSubConfigure->addSeparator();

    if (TRUE == ExtendedConfigActive) {
      trayMenuSubSpecialConfigure = trayMenuSubConfigure->addMenu(tr("Special Configure"));
      trayMenuSubSpecialConfigure->addAction(Stick20ActionFillSDCardWithRandomChars);

      if (TRUE == SdCardNotErased)
        trayMenuSubSpecialConfigure->addAction(Stick20ActionClearNewSDCardFound);
    }

    // Enable "reset user PIN" ?
    if (0 == libada::i()->getUserPasswordRetryCount()) {
      trayMenu->addSeparator();
      trayMenu->addAction(Stick20ActionResetUserPassword);
    }

    // Add debug window ?
    if (TRUE == DebugWindowActive) {
      trayMenu->addSeparator();
      trayMenu->addAction(Stick20ActionDebugAction);
    }

    // Setup OTP combo box
    generateComboBoxEntrys();
  }
}

void MainWindow::generateHOTPConfig(OTPSlot *slot) {
  uint8_t selectedSlot = ui->slotComboBox->currentIndex();
  selectedSlot -= (TOTP_SlotCount + 1);

  if (selectedSlot < HOTP_SlotCount) {
    slot->slotNumber = selectedSlot + 0x10;

    generateOTPConfig(slot);


    memset(slot->counter, 0, 8);
    // Nitrokey Storage needs counter value in text but Pro in binary [#60]
    if (libada::i()->isStorageDeviceConnected() == false) {
      bool conversionSuccess = false;
      uint64_t counterFromGUI = 0;
      if (0 != ui->counterEdit->text().toLatin1().length()) {
        counterFromGUI = ui->counterEdit->text().toLatin1().toULongLong(&conversionSuccess);
      }
      if (conversionSuccess) {
        // FIXME check for little endian/big endian conversion (test on Macintosh)
        memcpy(slot->counter, &counterFromGUI, sizeof counterFromGUI);
      } else {
          csApplet()->warningBox(tr("Counter value not copied - there was an error in conversion. "
                                            "Setting counter value to 0. Please retry."));
      }
    } else { // nitrokey storage version
      QByteArray counterFromGUI = QByteArray(ui->counterEdit->text().toLatin1());
      int digitsInCounter = counterFromGUI.length();
      if (0 < digitsInCounter && digitsInCounter < 8) {
        memcpy(slot->counter, counterFromGUI.constData(), std::min(counterFromGUI.length(), 7));
        // 8th char has to be '\0' since in firmware atoi is used directly on buffer
        slot->counter[7] = 0;
      } else {
          csApplet()->warningBox(tr("Counter value not copied - Nitrokey Storage handles HOTP counter "
                                            "values up to 7 digits. Setting counter value to 0. Please retry."));
      }
    }

#ifdef DEBUG
    if (DebugingActive) {
      if (cryptostick->activStick20)
        qDebug() << "HOTP counter value: " << *reinterpret_cast<char *>(slot->counter);
      else
        qDebug() << "HOTP counter value: " << *reinterpret_cast<quint64 *> (slot->counter);
    }
#endif

  }
}


void MainWindow::generateOTPConfig(OTPSlot *slot) const {
  QByteArray secretFromGUI = this->ui->secretEdit->text().toLatin1();

  uint8_t encoded[128] = {};
  uint8_t decoded[2*SECRET_LENGTH] = {};
  uint8_t data[128] = {};

  memset(encoded, 'A', sizeof(encoded));
  memset(data, 'A', sizeof(data));
  size_t toCopy;
  toCopy = std::min(sizeof(encoded), (const size_t &) secretFromGUI.length());
  memcpy(encoded, secretFromGUI.constData(), toCopy);

  base32_clean(encoded, sizeof(data), data);
  base32_decode(data, decoded, sizeof(decoded));

  secretFromGUI = QByteArray((char *)decoded, SECRET_LENGTH); // .toHex();
  memset(slot->secret, 0, sizeof(slot->secret));
  toCopy = std::min(sizeof(slot->secret), (const size_t &) secretFromGUI.length());
  memcpy(slot->secret, secretFromGUI.constData(), toCopy);

  QByteArray slotNameFromGUI = QByteArray(this->ui->nameEdit->text().toLatin1());
  memset(slot->slotName, 0, sizeof(slot->slotName));
  toCopy = std::min(sizeof(slot->slotName), (const size_t &) slotNameFromGUI.length());
  memcpy(slot->slotName, slotNameFromGUI.constData(), toCopy);

  memset(slot->tokenID, 32, sizeof(slot->tokenID));
  QByteArray ompFromGUI = (this->ui->ompEdit->text().toLatin1());
  toCopy = std::min(2ul, (const unsigned long &) ompFromGUI.length());
  memcpy(slot->tokenID, ompFromGUI.constData(), toCopy);

  QByteArray ttFromGUI = (this->ui->ttEdit->text().toLatin1());
  toCopy = std::min(2ul, (const unsigned long &) ttFromGUI.length());
  memcpy(slot->tokenID + 2, ttFromGUI.constData(), toCopy);

  QByteArray muiFromGUI = (this->ui->muiEdit->text().toLatin1());
  toCopy = std::min(8ul, (const unsigned long &) muiFromGUI.length());
  memcpy(slot->tokenID + 4, muiFromGUI.constData(), toCopy);

  slot->tokenID[12] = (uint8_t) (this->ui->keyboardComboBox->currentIndex() & 0xFF);

  slot->config = 0;
  if (ui->digits8radioButton->isChecked())
      slot->config += (1 << 0);
  if (ui->enterCheckBox->isChecked())
      slot->config += (1 << 1);
  if (ui->tokenIDCheckBox->isChecked())
      slot->config += (1 << 2);
}

void MainWindow::generateTOTPConfig(OTPSlot *slot) {
  uint8_t selectedSlot = ui->slotComboBox->currentIndex();

  // get the TOTP slot number
  if (selectedSlot < TOTP_SlotCount) {
    slot->slotNumber = selectedSlot + 0x20;

    generateOTPConfig(slot);

    uint16_t lastInterval = ui->intervalSpinBox->value();

    if (lastInterval < 1)
      lastInterval = 1;

    slot->interval = lastInterval;
  }
}

void MainWindow::generateAllConfigs() {
  displayCurrentSlotConfig();
  generateMenu();
}

void MainWindow::displayCurrentTotpSlotConfig(uint8_t slotNo) {
  ui->label_5->setText(tr("TOTP length:"));
  ui->label_6->hide();
  ui->counterEdit->hide();
  ui->setToZeroButton->hide();
  ui->setToRandomButton->hide();
  ui->enterCheckBox->hide();
  ui->labelNotify->hide();
  ui->intervalLabel->show();
  ui->intervalSpinBox->show();
  ui->checkBox->setEnabled(false);
  ui->secretEdit->setPlaceholderText("********************************");

  ui->nameEdit->setText(QString::fromStdString(libada::i()->getTOTPSlotName(slotNo)));
//  QByteArray secret((char *)cryptostick->TOTPSlots[slotNo]->secret, SECRET_LENGTH);

  ui->base32RadioButton->setChecked(true);
//  ui->secretEdit->setText(secret); // .toHex());

  ui->counterEdit->setText("0");

  //TODO readout TOTP slot data
//  QByteArray omp((char *)cryptostick->TOTPSlots[slotNo]->tokenID, 2);
//  ui->ompEdit->setText(QString(omp));

//  QByteArray tt((char *)cryptostick->TOTPSlots[slotNo]->tokenID + 2, 2);
//  ui->ttEdit->setText(QString(tt));

//  QByteArray mui((char *)cryptostick->TOTPSlots[slotNo]->tokenID + 4, 8);
//  ui->muiEdit->setText(QString(mui));

//  int interval = cryptostick->TOTPSlots[slotNo]->interval;
//  if (interval<1) interval = 30;
//  ui->intervalSpinBox->setValue(interval);

//  if (cryptostick->TOTPSlots[slotNo]->config & (1 << 0))
//    ui->digits8radioButton->setChecked(true);
//  else
//    ui->digits6radioButton->setChecked(true);

//  ui->enterCheckBox->setChecked((cryptostick->TOTPSlots[slotNo]->config & (1 << 1)));
//  ui->tokenIDCheckBox->setChecked(cryptostick->TOTPSlots[slotNo]->config & (1 << 2));

  if (!libada::i()->isTOTPSlotProgrammed(slotNo)) {
    ui->ompEdit->setText("NK");
    ui->ttEdit->setText("01");
//    QByteArray cardSerial = QByteArray((char *)cryptostick->cardSerial).toHex();
//    ui->muiEdit->setText(QString("%1").arg(QString(cardSerial), 8, '0'));
  }
}

void MainWindow::displayCurrentHotpSlotConfig(uint8_t slotNo) {
  ui->label_5->setText(tr("HOTP length:"));
  ui->label_6->show();
  ui->counterEdit->show();
  ui->setToZeroButton->show();
  ui->setToRandomButton->show();
  ui->enterCheckBox->show();
  ui->labelNotify->hide();
  ui->intervalLabel->hide();
  ui->intervalSpinBox->hide();
  ui->checkBox->setEnabled(false);
  ui->secretEdit->setPlaceholderText("********************************");

  // slotNo=slotNo+0x10;
  ui->nameEdit->setText(QString::fromStdString(libada::i()->getHOTPSlotName(slotNo)));
//  QByteArray secret((char *)cryptostick->HOTPSlots[slotNo]->secret, SECRET_LENGTH);
//  ui->secretEdit->setText(secret); // .toHex());

  ui->base32RadioButton->setChecked(true);

  //TODO get data for HOTP slot

//  if (libada::i()->isStorageDeviceConnected()) {
//    QByteArray counter((char *)cryptostick->HOTPSlots[slotNo]->counter, 8);
//    QString TextCount;
//    TextCount = QString("%1").arg(counter.toInt());
//    ui->counterEdit->setText(TextCount); // .toHex());
//  } else {
//    QString TextCount = QString("%1")
//        .arg(cryptostick->HOTPSlots[slotNo]->interval); //use 64bit integer from counters union
//    ui->counterEdit->setText(TextCount);
//  }
//
//  QByteArray omp((char *)cryptostick->HOTPSlots[slotNo]->tokenID, 2);
//  ui->ompEdit->setText(QString(omp));
//
//  QByteArray tt((char *)cryptostick->HOTPSlots[slotNo]->tokenID + 2, 2);
//  ui->ttEdit->setText(QString(tt));
//
//  QByteArray mui((char *)cryptostick->HOTPSlots[slotNo]->tokenID + 4, 8);
//  ui->muiEdit->setText(QString(mui));
//
//  if (cryptostick->HOTPSlots[slotNo]->config & (1 << 0))
//    ui->digits8radioButton->setChecked(true);
//  else
//    ui->digits6radioButton->setChecked(true);

//  ui->enterCheckBox->setChecked((cryptostick->HOTPSlots[slotNo]->config & (1 << 1)));

//  ui->tokenIDCheckBox->setChecked((cryptostick->HOTPSlots[slotNo]->config & (1 << 2)));

  if (!libada::i()->isHOTPSlotProgrammed(slotNo)) {
    ui->ompEdit->setText("NK");
    ui->ttEdit->setText("01");
//    QByteArray cardSerial = QByteArray((char *)cryptostick->cardSerial).toHex();
//    ui->muiEdit->setText(QString("%1").arg(QString(cardSerial), 8, '0'));
  }
}

void MainWindow::displayCurrentSlotConfig() {
  uint8_t slotNo = ui->slotComboBox->currentIndex();

  if (slotNo == 255)
    return;

  if (slotNo > TOTP_SlotCount)
    slotNo -= (TOTP_SlotCount + 1);
  else
    slotNo += HOTP_SlotCount;

  if (slotNo < HOTP_SlotCount)
    displayCurrentHotpSlotConfig(slotNo);
  else if ((slotNo >= HOTP_SlotCount) && (slotNo < HOTP_SlotCount + TOTP_SlotCount)) {
    slotNo -= HOTP_SlotCount;
    displayCurrentTotpSlotConfig(slotNo);
  }

  lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
}

void MainWindow::displayCurrentGeneralConfig() {
  ui->numLockComboBox->setCurrentIndex(0);
  ui->capsLockComboBox->setCurrentIndex(0);
  ui->scrollLockComboBox->setCurrentIndex(0);

//  if (cryptostick->generalConfig[0] == 0 || cryptostick->generalConfig[0] == 1)
//    ui->numLockComboBox->setCurrentIndex(cryptostick->generalConfig[0] + 1);
//
//  if (cryptostick->generalConfig[1] == 0 || cryptostick->generalConfig[1] == 1)
//    ui->capsLockComboBox->setCurrentIndex(cryptostick->generalConfig[1] + 1);
//
//  if (cryptostick->generalConfig[2] == 0 || cryptostick->generalConfig[2] == 1)
//    ui->scrollLockComboBox->setCurrentIndex(cryptostick->generalConfig[2] + 1);
//
//  ui->enableUserPasswordCheckBox->setChecked(cryptostick->otpPasswordConfig[0] == 1);
//  ui->deleteUserPasswordCheckBox->setChecked(cryptostick->otpPasswordConfig[1] == 1);

  lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
}

void MainWindow::startConfiguration() {
  //TODO authenticate admin plain
//  PinDialog dialog(tr("Enter card admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
  csApplet()->warningBox(tr("Wrong PIN. Please try again."));

  bool validPassword = true;
  // Start the config dialog
  if (validPassword) {
//    cryptostick->getSlotConfigs();
    displayCurrentSlotConfig();

//      translateDeviceStatusToUserMessage(cryptostick->getStatus()); //TODO
    displayCurrentGeneralConfig();

    SetupPasswordSafeConfig();

    raise();
    activateWindow();
    showNormal();
    setWindowState(Qt::WindowActive);

    QTimer::singleShot(0, this, SLOT(resizeMin()));
  }
  if (libada::i()->isStorageDeviceConnected()) {
    ui->counterEdit->setMaxLength(7);
  }
}

void MainWindow::resizeMin() { resize(minimumSizeHint()); }


void MainWindow::startStickDebug() {
  DebugDialog dialog(this);
//  dialog.cryptostick = cryptostick;
  dialog.updateText(); // Init data
  dialog.exec();
}

void MainWindow::refreshStick20StatusData() {
//  if (TRUE == libada::i()->isStorageDeviceConnected()) {
//    // Get actual data from stick 20
//    cryptostick->stick20GetStatusData();
//    Stick20ResponseTask ResponseTask(this, cryptostick, trayIcon);
//    ResponseTask.NoStopWhenStatusOK();
//    ResponseTask.GetResponse();
//    UpdateDynamicMenuEntrys(); // Use new data to update menu
//  }
}

void MainWindow::startAboutDialog() {
  AboutDialog dialog(this);
  dialog.exec();
}


void MainWindow::startStick20EnableCryptedVolume() {
  bool ret;
  bool answer;

  if (TRUE == HiddenVolumeActive) {
    answer = csApplet()->yesOrNoBox(tr("This activity locks your hidden volume. Do you want to "
                                               "proceed?\nTo avoid data loss, please unmount the partitions before "
                                               "proceeding."), false);
    if (false == answer)
      return;
  }

  PinDialog dialog(tr("User pin dialog"), tr("Enter user PIN:"), PinDialog::PREFIXED,
                   PinDialog::USER_PIN, this);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    local_sync();
    const auto s = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->unlock_encrypted_volume(s.data());
  }
}

void MainWindow::startStick20DisableCryptedVolume() {
  if (TRUE == CryptedVolumeActive) {
    bool answer = csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                               "proceed?\nTo avoid data loss, please unmount the partitions before "
                                               "proceeding."), false);
    if (false == answer)
      return;

    local_sync();
    auto m = nitrokey::NitrokeyManager::instance();
    m->lock_device();
  }
}

void MainWindow::startStick20EnableHiddenVolume() {
  bool ret;
  bool answer;

  if (FALSE == CryptedVolumeActive) {
      csApplet()->warningBox(tr("Please enable the encrypted volume first."));
    return;
  }

  answer =
          csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                            "proceed?\nTo avoid data loss, please unmount the partitions before "
                                            "proceeding."), true);
  if (false == answer)
    return;

  PinDialog dialog(tr("Enter password for hidden volume"), tr("Enter password for hidden volume:"),
                   PinDialog::PREFIXED, PinDialog::OTHER, this);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    local_sync();
    // password[0] = 'P';
    auto s = dialog.getPassword();

    auto m = nitrokey::NitrokeyManager::instance();
    m->unlock_hidden_volume(s.data());
  }
}

void MainWindow::startStick20DisableHiddenVolume() {
  bool answer =
          csApplet()->yesOrNoBox(tr("This activity locks your hidden volume. Do you want to proceed?\nTo "
                                            "avoid data loss, please unmount the partitions before proceeding."), true);
  if (false == answer)
    return;

  local_sync();
//  stick20SendCommand(STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI, password);
  auto m = nitrokey::NitrokeyManager::instance();
  m->lock_device();
}

void MainWindow::startLockDeviceAction() {
  bool answer;

  if ((TRUE == CryptedVolumeActive) || (TRUE == HiddenVolumeActive)) {
    answer = csApplet()->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                                               "proceed?\nTo avoid data loss, please unmount the partitions before "
                                               "proceeding."), true);
    if (false == answer) {
      return;
    }
    local_sync();
  }

  auto m = nitrokey::NitrokeyManager::instance();
  m->lock_device();

  PasswordSafeEnabled = FALSE;

  UpdateDynamicMenuEntrys();
  showTrayMessage("Nitrokey App", tr("Device has been locked"), INFORMATION, TRAY_MSG_TIMEOUT);
}

void MainWindow::startStick20EnableFirmwareUpdate() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool ret;

  UpdateDialog dialogUpdate(this);

  ret = dialogUpdate.exec();
  if (QDialog::Accepted != ret) {
    return;
  }

  PinDialog dialog(tr("Enter Firmware Password"), tr("Enter Firmware Password:"),
                   PinDialog::PREFIXED, PinDialog::FIRMWARE_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    // FIXME unmount all volumes and sync
    dialog.getPassword((char *)password);

    //TODO add firmware update logic
//    stick20SendCommand(STICK20_CMD_ENABLE_FIRMWARE_UPDATE, password);
//    auto m = nitrokey::NitrokeyManager::instance();
//    m->enable_firmware_update();
  }
}

void MainWindow::startStick10ActionChangeUserPIN() {
  DialogChangePassword dialog(this);
  dialog.setModal(TRUE);
  dialog.PasswordKind = STICK10_PASSWORD_KIND_USER;
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick10ActionChangeAdminPIN() {
  DialogChangePassword dialog(this);
  dialog.setModal(TRUE);
  dialog.PasswordKind = STICK10_PASSWORD_KIND_ADMIN;
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ActionChangeUpdatePIN() {
  DialogChangePassword dialog(this);
  dialog.setModal(TRUE);
  dialog.PasswordKind = STICK20_PASSWORD_KIND_UPDATE;
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ActionChangeUserPIN() {
  DialogChangePassword dialog(this);
  dialog.setModal(TRUE);
  dialog.PasswordKind = STICK20_PASSWORD_KIND_USER;
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ActionChangeAdminPIN() {
  DialogChangePassword dialog(this);
  dialog.setModal(TRUE);
  dialog.PasswordKind = STICK20_PASSWORD_KIND_ADMIN;
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startResetUserPassword() {
  DialogChangePassword dialog(this);
  dialog.setModal(TRUE);
  if (libada::i()->isStorageDeviceConnected())
    dialog.PasswordKind = STICK20_PASSWORD_KIND_RESET_USER;
  else
    dialog.PasswordKind = STICK10_PASSWORD_KIND_RESET_USER;

  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ExportFirmwareToFile() {
  uint8_t password[LOCAL_PASSWORD_SIZE];
  bool ret;

  PinDialog dialog(tr("Enter admin PIN"), tr("Enter admin PIN:"), PinDialog::PREFIXED,
                   PinDialog::ADMIN_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    // password[0] = 'P';
    auto s = dialog.getPassword();

    auto m = nitrokey::NitrokeyManager::instance();
    m->export_firmware(s.data());
  }
}

void MainWindow::startStick20DestroyCryptedVolume(int fillSDWithRandomChars) {
  int ret;
  bool answer;

  answer = csApplet()->yesOrNoBox(tr("WARNING: Generating new AES keys will destroy the encrypted volumes, "
                                             "hidden volumes, and password safe! Continue?"), false);
  if (true == answer) {
    PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), PinDialog::PREFIXED,
                     PinDialog::ADMIN_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret) {
      auto s = dialog.getPassword();

      auto m = nitrokey::NitrokeyManager::instance();
      m->build_aes_key(s.data());
      if (fillSDWithRandomChars != 0) {
        m->fill_SD_card_with_random_data(s.data());
      }
      refreshStick20StatusData();
    }
  }
}

void MainWindow::startStick20FillSDCardWithRandomChars() {
  bool ret;
  PinDialog dialog(tr("Enter admin PIN"), tr("Admin Pin:"), PinDialog::PREFIXED,
                   PinDialog::ADMIN_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    auto s = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->fill_SD_card_with_random_data(s.data());
  }
}

void MainWindow::startStick20ClearNewSdCardFound() {
  uint8_t password[LOCAL_PASSWORD_SIZE];
  bool ret;
  PinDialog dialog(tr("Enter admin PIN"), tr("Enter admin PIN:"), PinDialog::PREFIXED,
                   PinDialog::ADMIN_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    auto s = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->clear_new_sd_card_warning(s.data());
  }
}

void MainWindow::startStick20GetStickStatus() {
}

void MainWindow::startStick20SetReadOnlyUncryptedVolume() {
  bool ret;

  PinDialog dialog(tr("Enter user PIN"), tr("User PIN:"), PinDialog::PREFIXED,
                   PinDialog::USER_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    const auto pass = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->set_unencrypted_read_only(pass.data());
//    pass.safe_clear(); //TODO
  }
}

void MainWindow::startStick20SetReadWriteUncryptedVolume() {
  uint8_t password[LOCAL_PASSWORD_SIZE];
  bool ret;

  PinDialog dialog(tr("Enter user PIN"), tr("User PIN:"), PinDialog::PREFIXED,
                   PinDialog::USER_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    const auto pass = dialog.getPassword();
    auto m = nitrokey::NitrokeyManager::instance();
    m->set_unencrypted_read_write(pass.data());
//    pass.safe_clear(); //TODO
  }
}

void MainWindow::startStick20LockStickHardware() {
  uint8_t password[LOCAL_PASSWORD_SIZE];
  bool ret;
  stick20LockFirmwareDialog dialog(this);

  ret = dialog.exec();
  if (QDialog::Accepted == ret) {
    PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), PinDialog::PREFIXED,
                     PinDialog::ADMIN_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret) {
      const auto pass = dialog.getPassword();
      auto m = nitrokey::NitrokeyManager::instance();
//    pass.safe_clear(); //TODO
// TODO     stick20SendCommand(STICK20_CMD_SEND_LOCK_STICK_HARDWARE, password);
    }
  }
}

void MainWindow::startStick20SetupPasswordMatrix() {
}

void MainWindow::startStick20DebugAction() {
}

void MainWindow::startStick20SetupHiddenVolume() {
  bool ret;
  stick20HiddenVolumeDialog HVDialog(this);

  if (FALSE == CryptedVolumeActive) {
      csApplet()->warningBox(tr("Please enable the encrypted volume first."));
    return;
  }

//FIXME this should be called from HVDialog
  HVDialog.SdCardHighWatermark_Read_Min = 0;
  HVDialog.SdCardHighWatermark_Read_Max = 100;
  HVDialog.SdCardHighWatermark_Write_Min = 0;
  HVDialog.SdCardHighWatermark_Write_Max = 100;
//  ret = cryptostick->getHighwaterMarkFromSdCard(
//      &HVDialog.SdCardHighWatermark_Write_Min, &HVDialog.SdCardHighWatermark_Write_Max,
//      &HVDialog.SdCardHighWatermark_Read_Min, &HVDialog.SdCardHighWatermark_Read_Max);
//  HVDialog.setHighWaterMarkText();
  ret = HVDialog.exec();

  if (true == ret) {
    const auto d = HVDialog.HV_Setup_st;
    auto p = std::string( reinterpret_cast< char const* >(d.HiddenVolumePassword_au8));
    auto m = nitrokey::NitrokeyManager::instance();
    m->create_hidden_volume(d.SlotNr_u8, d.StartBlockPercent_u8,
                            d.EndBlockPercent_u8, p.data());
  }
}

int MainWindow::UpdateDynamicMenuEntrys(void) {
  auto m = nitrokey::NitrokeyManager::instance();
  if (!m->is_connected()) return FALSE;
  auto s = m->get_status_storage();

  NormalVolumeRWActive =
      s.ReadWriteFlagUncryptedVolume_u8 ? FALSE : TRUE;

  CryptedVolumeActive =
      s.VolumeActiceFlag_st.encrypted ? TRUE : FALSE;

  HiddenVolumeActive =
      s.VolumeActiceFlag_st.hidden ? TRUE : FALSE;

  StickNotInitated = s.StickKeysNotInitiated ? TRUE : FALSE;

  SdCardNotErased = s.SDFillWithRandomChars_u8 ? TRUE : FALSE;

  Stick20ScSdCardOnline = TRUE;

  generateMenu();

  return (TRUE);
}

void MainWindow::storage_check_symlink(){
    if (!QFileInfo("/dev/nitrospace").isSymLink()) {
        csApplet()->warningBox(tr("Warning: The encrypted Volume is not formatted.\n\"Use GParted "
                                          "or fdisk for this.\""));
    }
}

int MainWindow::stick20SendCommand(uint8_t stick20Command, uint8_t *password) {
//  csApplet()->warningBox(tr("There was an error during communicating with device. Please try again."));
//  csApplet()->yesOrNoBox(tr("This command fills the encrypted volumes with random data "
//                                "and will destroy all encrypted volumes!\n"
//                                "It requires more than 1 hour for 32GB. Do you want to continue?"), false);
//  csApplet()->warningBox(tr("Either the password is not correct or the command execution resulted "
//                                "in an error. Please try again."));
  return (true);
}

void MainWindow::on_writeButton_clicked() {
  uint8_t slotNo = (uint8_t) ui->slotComboBox->currentIndex();

  PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), PinDialog::PLAIN, PinDialog::ADMIN_PIN, this);

  if (slotNo > TOTP_SlotCount)
    slotNo -= (TOTP_SlotCount + 1);
  else
    slotNo += HOTP_SlotCount;

  if (ui->nameEdit->text().isEmpty()) {
    csApplet()->warningBox(tr("Please enter a slotname."));
    return;
  }

  if (libada::i()->isDeviceConnected()) {
    ui->base32RadioButton->toggle();

    OTPSlot otp;
    if (slotNo < HOTP_SlotCount) { // HOTP slot
      generateHOTPConfig(&otp);
    } else {
      generateTOTPConfig(&otp);
    }
    if (!validate_secret(otp.secret)) {
      return;
    }
    libada::i()->writeToOTPSlot(otp);

//          csApplet()->messageBox(tr("Configuration successfully written."));
//          csApplet()->warningBox(tr("The name of the slot must not be empty."));
//          csApplet()->warningBox(tr("Wrong PIN. Please try again."));
//          csApplet()->warningBox(tr("Error writing configuration!"));


    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    Sleep::msleep(500);
    QApplication::restoreOverrideCursor();

    generateAllConfigs();
  } else
    csApplet()->warningBox(tr("Nitrokey is not connected!"));

  displayCurrentSlotConfig();
}

bool MainWindow::validate_secret(const uint8_t *secret) const {
  if(libada::i()->is_nkpro_07_rtm1() && secret[0] == 0){
      csApplet()->warningBox(tr("Nitrokey Pro v0.7 does not support secrets starting from null byte. Please change the secret."));
    return false;
  }
  //check if the secret consist only from null bytes
  //(this value is reserved - it would be ignored by device)
  for (int i=0; i < SECRET_LENGTH; i++){
    if (secret[i] != 0){
      return true;
    }
  }
  csApplet()->warningBox(tr("Your secret is invalid. Please change the secret."));
  return false;
}

void MainWindow::on_slotComboBox_currentIndexChanged(int) {
  displayCurrentSlotConfig();
}

void MainWindow::on_hexRadioButton_toggled(bool checked) {
  if (!checked) {
    return;
  }
  ui->secretEdit->setMaxLength(get_supported_secret_length_hex());

  QByteArray secret;
  uint8_t encoded[SECRET_LENGTH_BASE32] = {};
  uint8_t data[SECRET_LENGTH_BASE32] = {};
  uint8_t decoded[SECRET_LENGTH] = {};

  secret = ui->secretEdit->text().toLatin1();
  if (secret.size() != 0) {
    const size_t encoded_size = std::min(sizeof(encoded), (size_t) secret.length());
    memset(encoded, 'A', sizeof(encoded));
    memcpy(encoded, secret.constData(), encoded_size);

    base32_clean(encoded, sizeof(encoded), data);
    const size_t decoded_size = sizeof(decoded);
    base32_decode(data, decoded, decoded_size);

    secret = QByteArray((char *) decoded, decoded_size).toHex();

    ui->secretEdit->setText(QString(secret));
    secretInClipboard = ui->secretEdit->text();
    copyToClipboard(secretInClipboard);
  }
}

int MainWindow::get_supported_secret_length_hex() const {
  auto local_secret_length = SECRET_LENGTH_HEX;
  if (!libada::i()->is_secret320_supported()){
    local_secret_length /= 2;
  }
  return local_secret_length;
}

void MainWindow::on_base32RadioButton_toggled(bool checked) {
  if (!checked) {
    return;
  }

  QByteArray secret;
  uint8_t encoded[SECRET_LENGTH_BASE32+1] = {}; //+1 for \0
  uint8_t decoded[SECRET_LENGTH] = {};

  secret = QByteArray::fromHex(ui->secretEdit->text().toLatin1());
  if (secret.size() != 0) {
    const size_t decoded_size = std::min(sizeof(decoded), (size_t) secret.length());
    memcpy(decoded, secret.constData(), decoded_size);
    base32_encode(decoded, decoded_size, encoded, sizeof(encoded));

    ui->secretEdit->setText(QString((char *) encoded));
    secretInClipboard = ui->secretEdit->text();
    copyToClipboard(secretInClipboard);
  }
  ui->secretEdit->setMaxLength(get_supported_secret_length_base32());
}

void MainWindow::on_setToZeroButton_clicked() { ui->counterEdit->setText("0"); }

void MainWindow::on_setToRandomButton_clicked() {
  quint64 counter;
  counter = qrand();
  if (libada::i()->isStorageDeviceConnected()) {
    const int maxDigits = 7;
    counter = counter % ((quint64)pow(10, maxDigits));
  }
  ui->counterEdit->setText(QString(QByteArray::number(counter, 10)));
}

void MainWindow::on_tokenIDCheckBox_toggled(bool checked) {
  if (checked) {
    ui->ompEdit->setEnabled(checked);
    ui->ttEdit->setEnabled(checked);
    ui->muiEdit->setEnabled(checked);
  }
}

void MainWindow::on_enableUserPasswordCheckBox_toggled(bool checked) {
  ui->deleteUserPasswordCheckBox->setEnabled(checked);
  //TODO read state of deleteUserPassword on check
//  cryptostick->otpPasswordConfig[0] = (uint8_t)checked;
//  ui->deleteUserPasswordCheckBox->setChecked(cryptostick->otpPasswordConfig[1]);
}

void MainWindow::on_writeGeneralConfigButton_clicked() {
  uint8_t data[5];

//  PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), PinDialog::PLAIN, PinDialog::ADMIN_PIN, this);

  if (libada::i()->isDeviceConnected()) {

    data[0] = ui->numLockComboBox->currentIndex() - 1;
    data[1] = ui->capsLockComboBox->currentIndex() - 1;
    data[2] = ui->scrollLockComboBox->currentIndex() - 1;

    data[3] = (uint8_t)(ui->enableUserPasswordCheckBox->isChecked() ? 1 : 0);
    data[4] = (uint8_t)(ui->deleteUserPasswordCheckBox->isChecked() &&
                                ui->enableUserPasswordCheckBox->isChecked()
                            ? 1
                            : 0);
    csApplet()->messageBox(tr("Configuration successfully written."));
    csApplet()->warningBox(tr("Wrong PIN. Please try again."));
    csApplet()->warningBox(tr("Error writing configuration!"));
    //TODO authorize

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    Sleep::msleep(500);
    QApplication::restoreOverrideCursor();
//      translateDeviceStatusToUserMessage(cryptostick->getStatus()); //TODO
    generateAllConfigs();
  } else {
      csApplet()->warningBox(tr("Nitrokey not connected!"));
  }
  displayCurrentGeneralConfig();
}

void MainWindow::getHOTPDialog(int slot) {
  int ret;

  ret = getNextCode(0x10 + slot);
  if (ret == 0) {
    if (libada::i()->getHOTPSlotName(slot).empty())
      showTrayMessage(QString(tr("HOTP slot ")).append(QString::number(slot + 1, 10)),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
    else
      showTrayMessage(QString(tr("HOTP slot "))
                          .append(QString::number(slot + 1, 10))
                          .append(" [")
                          .append(QString::fromStdString(libada::i()->getHOTPSlotName(slot)))
                          .append("]"),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
  }
}

void MainWindow::getTOTPDialog(int slot) {
  int ret;

  ret = getNextCode(0x20 + slot);
  if (ret == 0) {
    if (libada::i()->getTOTPSlotName(slot).empty())
        showTrayMessage(QString(tr("TOTP slot ")).append(QString::number(slot + 1, 10)),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
    else
      showTrayMessage(QString(tr("TOTP slot "))
                          .append(QString::number(slot + 1, 10))
                          .append(" [")
                          .append(QString::fromStdString(libada::i()->getTOTPSlotName(slot)))
                          .append("]"),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
  }
}

void MainWindow::on_eraseButton_clicked() {
  bool answer = csApplet()->yesOrNoBox(tr("WARNING: Are you sure you want to erase the slot?"), false);
  if (!answer) {
    return;
  }

  char clean[8] = {' '};
  uint8_t slotNo = ui->slotComboBox->currentIndex();

  if (slotNo > TOTP_SlotCount) {
    slotNo -= (TOTP_SlotCount + 1);
  } else {
    slotNo += HOTP_SlotCount;
  }

  if (slotNo < HOTP_SlotCount) {
    slotNo = slotNo + 0x10;
  } else if ((slotNo >= HOTP_SlotCount) && (slotNo <= TOTP_SlotCount + HOTP_SlotCount)) {
    slotNo = slotNo + 0x20 - HOTP_SlotCount;
  }

  //todo erase slot
//  int res = cryptostick->eraseSlot(slotNo);
  //todo catch exception not authorized
  if (libada::i()->is_nkpro_07_rtm1()) {
    uint8_t tempPassword[25] = {0};
    QString password;

    //TODO do authentication
//    do {
//      PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
//                       PinDialog::ADMIN_PIN);
//      int ok = dialog.exec();
//      if (ok != QDialog::Accepted) {
//        return;
//      }
//      dialog.getPassword(password);
//
//      generateTemporaryPassword(tempPassword);
//      cryptostick->firstAuthenticate((uint8_t *) password.toLatin1().data(), tempPassword);
//      if (cryptostick->validPassword) {
//        lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
//      } else {
//        csApplet()->warningBox(tr("Wrong PIN. Please try again."));
//      }
//      res = cryptostick->eraseSlot(slotNo);
//    } while (res != CMD_STATUS_OK);
  }
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  Sleep::msleep(1000);
  QApplication::restoreOverrideCursor();
  generateAllConfigs();

//  if (res == CMD_STATUS_OK) {
//    csApplet()->messageBox(tr("Slot has been erased successfully."));
//  } else {
//    csApplet()->messageBox(tr("Command execution failed. Please try again."));
//  }

  displayCurrentSlotConfig();
}

void MainWindow::on_randomSecretButton_clicked() {
  int i = 0;


  int local_secret_length = get_supported_secret_length_base32();
  uint8_t secret[local_secret_length];

  char temp;

  while (i < local_secret_length) {
    temp = qrand() & 0xFF;
    if ((temp >= 'A' && temp <= 'Z') || (temp >= '2' && temp <= '7')) {
      secret[i] = temp;
      i++;
    }
  }

  QByteArray secretArray((char *)secret, local_secret_length);

  ui->base32RadioButton->setChecked(true);
  ui->secretEdit->setText(secretArray);
  ui->checkBox->setEnabled(true);
  ui->checkBox->setChecked(true);
  secretInClipboard = ui->secretEdit->text();
  copyToClipboard(secretInClipboard);
}

int MainWindow::get_supported_secret_length_base32() const {
  auto local_secret_length = SECRET_LENGTH_BASE32;
  if (!libada::i()->is_secret320_supported()){
    local_secret_length /= 2;
  }
  return local_secret_length;
}

void MainWindow::on_checkBox_toggled(bool checked) {
  if (checked)
    ui->secretEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
  else
    ui->secretEdit->setEchoMode(QLineEdit::Normal);
}

void MainWindow::copyToClipboard(QString text) {
  if (text.length() != 0) {
    lastClipboardTime = QDateTime::currentDateTime().toTime_t();
    clipboard->setText(text);
    ui->labelNotify->show();
  }
}

void MainWindow::checkClipboard_Valid(bool ignore_time) {
  uint64_t currentTime, far_future_delta = 60000;

  currentTime = QDateTime::currentDateTime().toTime_t();
  if (ignore_time)
    currentTime += far_future_delta;
  if ((currentTime >= (lastClipboardTime + (uint64_t)60)) &&
      (clipboard->text() == otpInClipboard)) {
    overwrite_string(otpInClipboard);
    clipboard->setText(QString(""));
  }

  if ((currentTime >= (lastClipboardTime + (uint64_t)60)) &&
      (clipboard->text() == PWSInClipboard)) {
    overwrite_string(PWSInClipboard);
    clipboard->setText(QString(""));
  }

  if ((currentTime >= (lastClipboardTime + (uint64_t)120)) &&
      (clipboard->text() == secretInClipboard)) {
    overwrite_string(secretInClipboard);
    clipboard->setText(QString(""));
    ui->labelNotify->hide();
  }

  if (QString::compare(clipboard->text(), secretInClipboard) != 0) {
    ui->labelNotify->hide();
  }
}

void MainWindow::checkPasswordTime_Valid() {
  uint64_t currentTime;
  currentTime = QDateTime::currentDateTime().toTime_t();

  if(!nitrokey::NitrokeyManager::instance()->is_connected())
    return;

  auto status = nitrokey::NitrokeyManager::instance()->get_status();

  bool is_OTP_PIN_protected = status.enable_user_password;
  bool is_forget_PIN_after_10_minutes_enabled = status.delete_user_password;

  // invalidate admin authentication after 10 minutes
  if (currentTime >= lastAuthenticateTime + (uint64_t)600) {
    //todo invalidate password
//    cryptostick->validPassword = false;
//    memset(cryptostick->adminTemporaryPassword, 0, 25);
  }

  // invalidate user authentication after 10 minutes
  //(only when OTP is PIN protected and forget PIN is enabled)
  if (currentTime >= lastUserAuthenticateTime + (uint64_t)600 && is_OTP_PIN_protected &&
      is_forget_PIN_after_10_minutes_enabled) {
    //todo invalidate password
//    cryptostick->validUserPassword = false;
//    memset(cryptostick->userTemporaryPassword, 0, 25);
    overwrite_string(nkpro_user_PIN); // for NK Pro 0.7 only
  }
}

void MainWindow::checkTextEdited() {
  ui->secretEdit->setText(ui->secretEdit->text().remove(" ").remove("\t"));
  if (!ui->checkBox->isEnabled()) {
    ui->checkBox->setEnabled(true);
    ui->checkBox->setChecked(false);
  }
}

void MainWindow::SetupPasswordSafeConfig(void) {
  int ret;

  int i;

  QString Slotname;

  ui->PWS_ComboBoxSelectSlot->clear();
  PWS_Access = FALSE;

  // Get active password slots
//  ret = cryptostick->getPasswordSafeSlotStatus();
  ret = libada::i()->getPWSSlotStatus(0);
  const auto no_err = true; //TODO ERR_NO_ERROR == ret;
  if (no_err) {
    PWS_Access = TRUE;
    // Setup combobox
    for (i = 0; i < PWS_SLOT_COUNT; i++) {
      if (libada::i()->getPWSSlotStatus(i)) {
        ui->PWS_ComboBoxSelectSlot->addItem(
            QString(tr("Slot "))
                .append(QString::number(i + 1, 10))
                .append(QString(" [")
                            .append(QString::fromStdString(libada::i()->getPWSSlotName(i)))
                            .append(QString("]"))));
      } else {
        ui->PWS_ComboBoxSelectSlot->addItem(
            QString(tr("Slot ")).append(QString::number(i + 1, 10)));
      }
    }
  } else {
    ui->PWS_ComboBoxSelectSlot->addItem(QString(tr("Unlock password safe")));
  }

  if (TRUE == PWS_Access) {
    ui->PWS_ButtonEnable->hide();
    ui->PWS_ButtonSaveSlot->setEnabled(TRUE);
    ui->PWS_ButtonClearSlot->setEnabled(TRUE);

    ui->PWS_ComboBoxSelectSlot->setEnabled(TRUE);
    ui->PWS_EditSlotName->setEnabled(TRUE);
    ui->PWS_EditLoginName->setEnabled(TRUE);
    ui->PWS_EditPassword->setEnabled(TRUE);
    ui->PWS_CheckBoxHideSecret->setEnabled(TRUE);
    ui->PWS_ButtonCreatePW->setEnabled(TRUE);
  } else {
    ui->PWS_ButtonEnable->show();
    ui->PWS_ButtonSaveSlot->setDisabled(TRUE);
    ui->PWS_ButtonClearSlot->setDisabled(TRUE);

    ui->PWS_ComboBoxSelectSlot->setDisabled(TRUE);
    ui->PWS_EditSlotName->setDisabled(TRUE);
    ui->PWS_EditLoginName->setDisabled(TRUE);
    ui->PWS_EditPassword->setDisabled(TRUE);
    ui->PWS_CheckBoxHideSecret->setDisabled(TRUE);
    ui->PWS_ButtonCreatePW->setDisabled(TRUE);
  }

  ui->PWS_EditSlotName->setMaxLength(PWS_SLOTNAME_LENGTH);
  ui->PWS_EditPassword->setMaxLength(PWS_PASSWORD_LENGTH);
  ui->PWS_EditLoginName->setMaxLength(PWS_LOGINNAME_LENGTH);

  ui->PWS_CheckBoxHideSecret->setChecked(TRUE);
  ui->PWS_EditPassword->setEchoMode(QLineEdit::Password);
}

void MainWindow::on_PWS_ButtonClearSlot_clicked() {
  bool answer = csApplet()->yesOrNoBox(tr("WARNING: Are you sure you want to erase the slot?"), false);
  if (!answer){
      return;
  }

  int Slot;
  unsigned int ret;
  QMessageBox msgBox;

  Slot = ui->PWS_ComboBoxSelectSlot->currentIndex();
  if (libada::i()->getPWSSlotStatus(Slot)) // Is slot active?
  {
    try{
      libada::i()->erasePWSSlot(Slot);

      ui->PWS_EditSlotName->setText("");
      ui->PWS_EditPassword->setText("");
      ui->PWS_EditLoginName->setText("");
      ui->PWS_ComboBoxSelectSlot->setItemText(
          Slot, QString("Slot ").append(QString::number(Slot + 1, 10)));
      csApplet()->messageBox(tr("Slot has been erased successfully."));
    }
    catch (std::exception &e){
      csApplet()->warningBox(tr("Can't clear slot."));
    }
  } else
      csApplet()->messageBox(tr("Slot is erased already."));

  generateMenu();
}

void MainWindow::on_PWS_ComboBoxSelectSlot_currentIndexChanged(int index) {
  QString OutputText;

  if (FALSE == PWS_Access) {
    return;
  }

  // Slot already used ?
  if (libada::i()->getPWSSlotStatus(index)) {
    ui->PWS_EditSlotName->setText(QString::fromStdString(libada::i()->getPWSSlotName(index)));

    //TODO
//    ui->PWS_EditPassword->setText((QString)(char *)cryptostick->passwordSafePassword);
//    ui->PWS_EditLoginName->setText((QString)(char *)cryptostick->passwordSafeLoginName);
  } else {
    ui->PWS_EditSlotName->setText("");
    ui->PWS_EditPassword->setText("");
    ui->PWS_EditLoginName->setText("");
  }
}

void MainWindow::on_PWS_CheckBoxHideSecret_toggled(bool checked) {
  if (checked)
    ui->PWS_EditPassword->setEchoMode(QLineEdit::Password);
  else
    ui->PWS_EditPassword->setEchoMode(QLineEdit::Normal);
}

void MainWindow::on_PWS_ButtonSaveSlot_clicked() {
//  int Slot;
//  int ret;
//
//  uint8_t SlotName[PWS_SLOTNAME_LENGTH + 1];
//  uint8_t LoginName[PWS_LOGINNAME_LENGTH + 1];
//  uint8_t Password[PWS_PASSWORD_LENGTH + 1];
//
//  QMessageBox msgBox;
//
//  Slot = ui->PWS_ComboBoxSelectSlot->currentIndex();
//
//  STRNCPY((char *)SlotName, sizeof(SlotName), ui->PWS_EditSlotName->text().toUtf8(),
//          PWS_SLOTNAME_LENGTH);
//  SlotName[PWS_SLOTNAME_LENGTH] = 0;
//  if (0 == strlen((char *)SlotName)) {
//      csApplet()->warningBox(tr("Please enter a slotname."));
//    return;
//  }
//
//  STRNCPY((char *)LoginName, sizeof(LoginName), ui->PWS_EditLoginName->text().toUtf8(),
//          PWS_LOGINNAME_LENGTH);
//  LoginName[PWS_LOGINNAME_LENGTH] = 0;
//
//  STRNCPY((char *)Password, sizeof(Password), ui->PWS_EditPassword->text().toUtf8(),
//          PWS_PASSWORD_LENGTH);
//  Password[PWS_PASSWORD_LENGTH] = 0;
//  if (0 == strlen((char *)Password)) {
//      csApplet()->warningBox(tr("Please enter a password."));
//    return;
//  }
//
//  ret = cryptostick->setPasswordSafeSlotData_1(Slot, (uint8_t *)SlotName, (uint8_t *)Password);
//  if (ERR_NO_ERROR != ret) {
//    msgBox.setText(tr("Can't save slot. %1").arg(ret));
//    msgBox.exec();
//    return;
//  }
//
//  ret = cryptostick->setPasswordSafeSlotData_2(Slot, (uint8_t *)LoginName);
//  if (ERR_NO_ERROR != ret) {
//      csApplet()->warningBox(tr("Can't save slot."));
//    return;
//  }
//
//  cryptostick->passwordSafeStatus[Slot] = TRUE;
//  STRCPY((char *)cryptostick->passwordSafeSlotNames[Slot],
//         sizeof(cryptostick->passwordSafeSlotNames[Slot]), (char *)SlotName);
//  ui->PWS_ComboBoxSelectSlot->setItemText(
//      Slot, QString(tr("Slot "))
//                .append(QString::number(Slot + 1, 10))
//                .append(QString(" [")
//                            .append((char *)cryptostick->passwordSafeSlotNames[Slot])
//                            .append(QString("]"))));
//
//  generateMenu();
//  csApplet()->messageBox(tr("Slot successfully written."));
}

char *MainWindow::PWS_GetSlotName(int Slot) {
//  if (0 == strlen((char *)cryptostick->passwordSafeSlotNames[Slot])) {
//    cryptostick->getPasswordSafeSlotName(Slot);
//
//    STRCPY((char *)cryptostick->passwordSafeSlotNames[Slot],
//           sizeof(cryptostick->passwordSafeSlotNames[Slot]),
//           (char *)cryptostick->passwordSafeSlotName);
//  }
//  return ((char *)cryptostick->passwordSafeSlotNames[Slot]);
}

void MainWindow::on_PWS_ButtonClose_pressed() { hide(); }

void MainWindow::generateMenuPasswordSafe() {
//  if (FALSE == cryptostick->passwordSafeUnlocked) {
//    {
//      trayMenu->addAction(UnlockPasswordSafeAction);
//
//      UnlockPasswordSafeAction->setEnabled(true == cryptostick->passwordSafeAvailable);
//    }
//  }
}

void MainWindow::PWS_Clicked_EnablePWSAccess() {
//  uint8_t password[LOCAL_PASSWORD_SIZE];
//  bool ret;
//  int ret_s32;
//
//  QMessageBox msgBox;
//  PasswordDialog dialog(FALSE, this);
//
//  cryptostick->getUserPasswordRetryCount();
//  dialog.init((char *)(tr("Enter user PIN").toUtf8().data()),
//              HID_Stick20Configuration_st.UserPwRetryCount);
//  dialog.cryptostick = cryptostick;
//
//  ret = dialog.exec();
//
//  if (QDialog::Accepted == ret) {
//    dialog.getPassword((char *)password);
//
//    ret_s32 = cryptostick->isAesSupported((uint8_t *)&password[1]);
//
//    if (CMD_STATUS_OK == ret_s32) // AES supported, continue
//    {
//      cryptostick->passwordSafeAvailable = TRUE;
//      UnlockPasswordSafeAction->setEnabled(true);
//
//      // Continue to unlocking password safe
//      ret_s32 = cryptostick->passwordSafeEnable((char *)&password[1]);
//
//      if (ERR_NO_ERROR != ret_s32) {
//        switch (ret_s32) {
//        case CMD_STATUS_AES_DEC_FAILED:
//          uint8_t admin_password[SECRET_LENGTH_HEX];
//          cryptostick->getUserPasswordRetryCount();
//          dialog.init((char *)(tr("Enter admin PIN").toUtf8().data()),
//                      HID_Stick20Configuration_st.UserPwRetryCount);
//          dialog.cryptostick = cryptostick;
//          ret = dialog.exec();
//
//          if (QDialog::Accepted == ret) {
//            dialog.getPassword((char *)admin_password);
//
//            ret_s32 = cryptostick->buildAesKey((uint8_t *)&(admin_password[1]));
//
//            if (CMD_STATUS_OK != ret_s32) {
//              if (CMD_STATUS_WRONG_PASSWORD == ret_s32)
//                msgBox.setText(tr("Wrong password"));
//              else
//                msgBox.setText(tr("Unable to create new AES key"));
//
//              msgBox.exec();
//            } else {
//              Sleep::msleep(3000);
//              cryptostick->passwordSafeEnable((char *)&password[1]);
//            }
//          }
//
//          break;
//        default:
//            csApplet()->warningBox(tr("Can't unlock password safe."));
//          break;
//        }
//      } else {
//        PasswordSafeEnabled = TRUE;
//        showTrayMessage("Nitrokey App", tr("Password Safe unlocked successfully."), INFORMATION,
//                        TRAY_MSG_TIMEOUT);
//        SetupPasswordSafeConfig();
//        generateMenu();
//        ui->tabWidget->setTabEnabled(3, 1);
//      }
//    } else {
//      if (CMD_STATUS_NOT_SUPPORTED == ret_s32) // AES NOT supported
//      {
//        // Mark password safe as disabled feature
//        cryptostick->passwordSafeAvailable = FALSE;
//        UnlockPasswordSafeAction->setEnabled(false);
//          csApplet()->warningBox(tr("Password safe is not supported by this device."));
//        generateMenu();
//        ui->tabWidget->setTabEnabled(3, 0);
//      } else {
//        if (CMD_STATUS_WRONG_PASSWORD == ret_s32) // Wrong password
//        {
//            csApplet()->warningBox(tr("Wrong user password."));
//        }
//      }
//      return;
//    }
//  }
}

void MainWindow::PWS_ExceClickedSlot(int Slot) {
//  QString password_safe_password;
//
//  QString MsgText_1;
//
//  int ret_s32;
//
//  ret_s32 = cryptostick->getPasswordSafeSlotPassword(Slot);
//  if (ERR_NO_ERROR != ret_s32) {
//      csApplet()->warningBox(tr("Pasword safe: Can't get password"));
//    return;
//  }
//  password_safe_password.append((char *)cryptostick->passwordSafePassword);
//
//  PWSInClipboard = password_safe_password;
//  copyToClipboard(password_safe_password);
//
//  memset(cryptostick->passwordSafePassword, 0, sizeof(cryptostick->passwordSafePassword));
//
//  if (TRUE == trayIcon->supportsMessages()) {
//    password_safe_password =
//        QString(tr("Password safe [%1]").arg((char *)cryptostick->passwordSafeSlotNames[Slot]));
//    MsgText_1 = QString("Password has been copied to clipboard");
//
//    showTrayMessage(password_safe_password, MsgText_1, INFORMATION, TRAY_MSG_TIMEOUT);
//  } else {
//    password_safe_password = QString("Password safe [%1] has been copied to clipboard")
//                                 .arg((char *)cryptostick->passwordSafeSlotNames[Slot]);
//      csApplet()->messageBox(password_safe_password);
//  }
}

void MainWindow::resetTime() {
//  bool ok;
//
//  if (!cryptostick->validPassword) {
//
//    do {
//      PinDialog dialog(tr("Enter card admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
//                       PinDialog::ADMIN_PIN);
//      ok = dialog.exec();
//      QString password;
//
//      dialog.getPassword(password);
//
//      if (QDialog::Accepted == ok) {
//        uint8_t tempPassword[25];
//
//        for (int i = 0; i < 25; i++)
//          tempPassword[i] = qrand() & 0xFF;
//
//        cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(), tempPassword);
//        if (cryptostick->validPassword) {
//          lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
//        } else {
//            csApplet()->warningBox(tr("Wrong Pin. Please try again."));
//        }
//        password.clear();
//      }
//    } while (QDialog::Accepted == ok && !cryptostick->validPassword);
//  }
//
//  // Start the config dialog
//  if (cryptostick->validPassword) {
//    cryptostick->setTime(TOTP_SET_TIME);
//  }
}

int MainWindow::getNextCode(uint8_t slotNumber) {
//  uint8_t result[18] = {0};
//  uint32_t code;
//  uint8_t config;
//  int ret;
//  int ok;
//  uint16_t lastInterval = 30;
//
//    translateDeviceStatusToUserMessage(cryptostick->getStatus());
//  bool is_OTP_PIN_protected = cryptostick->otpPasswordConfig[0] == 1;
//  if (is_OTP_PIN_protected) {
//    if (!cryptostick->validUserPassword) {
//      cryptostick->getUserPasswordRetryCount();
//
//      PinDialog dialog(tr("Enter card user PIN"), tr("User PIN:"), cryptostick, PinDialog::PLAIN,
//                       PinDialog::USER_PIN);
//      ok = dialog.exec();
//      QString password;
//      dialog.getPassword(password);
//
//      if (cryptostick->is_nkpro_07_rtm1()) {
//        nkpro_user_PIN = password;
//      }
//
//      if (QDialog::Accepted == ok) {
//        userAuthenticate(password);
//      }
//      overwrite_string(password);
//      if (QDialog::Accepted != ok) {
//        return 1; // user does not click OK button
//      }
//    } else { // valid user password
//      if (cryptostick->is_nkpro_07_rtm1()) {
//        userAuthenticate(nkpro_user_PIN);
//      }
//    }
//  }
//  if (is_OTP_PIN_protected && ok == QDialog::Accepted && !cryptostick->validUserPassword) {
//      csApplet()->warningBox(tr("Invalid password!"));
//    return 1;
//  }
//
//  // Start the config dialog
//  // is it TOTP?
//  if (slotNumber >= 0x20)
//    cryptostick->TOTPSlots[slotNumber - 0x20]->interval = lastInterval;
//
//  lastTOTPTime = QDateTime::currentDateTime().toTime_t();
//  ret = cryptostick->setTime(TOTP_CHECK_TIME);
//
//  // if time is out of sync on the device
//  if (ret == -2) {
//    bool answer;
//    answer = csApplet()->detailedYesOrNoBox(tr("Time is out-of-sync"),
//                                            tr("WARNING!\n\nThe time of your computer and Nitrokey are out of "
//                                                       "sync.\nYour computer may be configured with a wrong time "
//                                                       "or\nyour Nitrokey may have been attacked. If an attacker "
//                                                       "or\nmalware could have used your Nitrokey you should reset the "
//                                                       "secrets of your configured One Time Passwords. If your "
//                                                       "computer's time is wrong, please configure it correctly and "
//                                                       "reset the time of your Nitrokey.\n\nReset Nitrokey's time?"),
//                                            false);
//
//    if (answer) {
//      resetTime();
//      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//      Sleep::msleep(1000);
//      QApplication::restoreOverrideCursor();
//      generateAllConfigs();
//        csApplet()->messageBox(tr("Time reset!"));
//    } else
//      return 1;
//  }
//
//  ret = cryptostick->getCode(slotNumber, lastTOTPTime / lastInterval, lastTOTPTime, lastInterval, result);
//  if(ret!=0){
//      //show error message
//      csApplet()->warningBox(
//        tr("Detected some communication problems with the device.")
//                  +QString(" ")+tr("Cannot get OTP code.")
//      );
//      return ret;
//  }
//  code = result[0] + (result[1] << 8) + (result[2] << 16) + (result[3] << 24);
//  config = result[4];
//
//  QString output;
//  if (config & (1 << 0)) {
//    code = code % 100000000;
//    output.append(QString("%1").arg(QString::number(code), 8, '0'));
//  } else {
//    code = code % 1000000;
//    output.append(QString("%1").arg(QString::number(code), 6, '0'));
//  }
//
//  otpInClipboard = output;
//  copyToClipboard(otpInClipboard);
//  if (DebugingActive)
//    qDebug() << otpInClipboard;
//
//  return 0;
}

int MainWindow::userAuthenticate(const QString &password) {
//  uint8_t tempPassword[25];
//  generateTemporaryPassword(tempPassword);
//  int result = cryptostick->userAuthenticate((uint8_t *)password.toLatin1().data(), tempPassword);
//  if (cryptostick->validUserPassword)
//    lastUserAuthenticateTime = QDateTime::currentDateTime().toTime_t();
//  return result;
}

void MainWindow::generateTemporaryPassword(uint8_t *tempPassword) const {
  for (int i = 0; i < 25; i++)
   tempPassword[i] = qrand() & 0xFF;
}

#define PWS_RANDOM_PASSWORD_CHAR_SPACE                                                             \
  "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"$%&/"                          \
  "()=?[]{}~*+#_'-`,.;:><^|@\\"

void MainWindow::on_PWS_ButtonCreatePW_clicked() {
  int i;

  int n;

  int PasswordCharSpaceLen;

  char RandomPassword[30];

  const char *PasswordCharSpace = PWS_RANDOM_PASSWORD_CHAR_SPACE;

  QString Text;

  PasswordCharSpaceLen = strlen(PasswordCharSpace);

  PWS_CreatePWSize = 20;
  for (i = 0; i < PWS_CreatePWSize; i++) {
    n = qrand();
    n = n % PasswordCharSpaceLen;
    RandomPassword[i] = PasswordCharSpace[n];
  }
  RandomPassword[i] = 0;

  Text = RandomPassword;
  ui->PWS_EditPassword->setText(Text.toLocal8Bit());
}

void MainWindow::on_PWS_ButtonEnable_clicked() { PWS_Clicked_EnablePWSAccess(); }

void MainWindow::on_counterEdit_editingFinished() {
//  bool conversionSuccess = false;
//  ui->counterEdit->text().toLatin1().toULongLong(&conversionSuccess);
//  if (cryptostick->activStick20 == false) {
//    quint64 counterMaxValue = ULLONG_MAX;
//    if (!conversionSuccess) {
//      ui->counterEdit->setText(QString("%1").arg(0));
//        csApplet()->warningBox(tr("Counter must be a value between 0 and %1").arg(counterMaxValue));
//    }
//  } else { // for nitrokey storage
//    if (!conversionSuccess || ui->counterEdit->text().toLatin1().length() > 7) {
//      ui->counterEdit->setText(QString("%1").arg(0));
//        csApplet()->warningBox(tr("For Nitrokey Storage counter must be a value between 0 and 9999999"));
//    }
//  }
}

char *MainWindow::getFactoryResetMessage(int retCode) {
  switch (retCode) {
//  case CMD_STATUS_OK:
//    return strdup("Factory reset was successful.");
//    break;
//  case CMD_STATUS_WRONG_PASSWORD:
//    return strdup("Wrong Pin. Please try again.");
//    break;
  default:
    return strdup("Unknown error.");
    break;
  }
}

int MainWindow::factoryReset() {
//  int ret;
//  bool ok;
//
//  do {
//    PinDialog dialog(tr("Enter card admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
//                     PinDialog::ADMIN_PIN);
//    ok = dialog.exec();
//    char password[LOCAL_PASSWORD_SIZE];
//    dialog.getPassword(password);
//    if (QDialog::Accepted == ok) {
//      ret = cryptostick->factoryReset(password);
//        csApplet()->messageBox(tr(getFactoryResetMessage(ret)));
//      memset(password, 0, strlen(password));
//    }
//  } while (QDialog::Accepted == ok && CMD_STATUS_WRONG_PASSWORD == ret);
//  // Disable pwd safe menu entries
//  int i;
//
//  for (i = 0; i <= 15; i++)
//    cryptostick->passwordSafeStatus[i] = false;
//
//  // Fetch configs from device
//  generateAllConfigs();
  return 0;
}

void MainWindow::on_radioButton_2_toggled(bool checked) {
  if (checked)
    ui->slotComboBox->setCurrentIndex(0);
}

void MainWindow::on_radioButton_toggled(bool checked) {
  if (checked)
    ui->slotComboBox->setCurrentIndex(TOTP_SlotCount + 1);
}

void setCounter(int size, const QString &arg1, QLabel *counter) {
  int chars_left = size - arg1.toUtf8().size();
  QString t = QString::number(chars_left);
  counter->setText(t);
}

void MainWindow::on_PWS_EditSlotName_textChanged(const QString &arg1) {
  setCounter(PWS_SLOTNAME_LENGTH, arg1, ui->l_c_name);
}

void MainWindow::on_PWS_EditLoginName_textChanged(const QString &arg1) {
  setCounter(PWS_LOGINNAME_LENGTH, arg1, ui->l_c_login);
}

void MainWindow::on_PWS_EditPassword_textChanged(const QString &arg1) {
  setCounter(PWS_PASSWORD_LENGTH, arg1, ui->l_c_password);
}

void MainWindow::on_enableUserPasswordCheckBox_clicked(bool checked) {
//  if (checked && cryptostick->is_nkpro_07_rtm1()) {
//    bool answer = csApplet()->yesOrNoBox(tr("To handle this functionality "
//                                                    "application will keep your user PIN in memory. "
//                                                    "Do you want to continue?"), false);
//    ui->enableUserPasswordCheckBox->setChecked(answer);
//  }
}
