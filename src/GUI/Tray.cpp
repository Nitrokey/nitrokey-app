//
// Created by sz on 14.01.17.
//

#include "Tray.h"
#include <QThread>
#include "libada.h"
#include "bool_values.h"
#include "nitrokey-applet.h"

Tray::Tray(QObject *_parent, bool _debug_mode){
  main_window = _parent;
  debug_mode = _debug_mode;

  createIndicator();
  initActionsForStick10();
  initActionsForStick20();
  initCommonActions();
}

Tray::~Tray() {
}


QThread thread_tray_populateOTP;

void tray_Worker::doWork() {
  //populate OTP name cache
  for (int i=0; i < TOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getTOTPSlotName(i);
  }

  for (int i=0; i<HOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getHOTPSlotName(i);
  }
  emit resultReady();
}


/*
 * Create the tray menu.
 * In all other systems we use Qt's tray
 */
void Tray::createIndicator() {
  trayIcon = new QSystemTrayIcon(this);
  trayIcon->setIcon(QIcon(":/images/CS_icon.png"));
  connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
          SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

  trayIcon->show();

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
  qDebug() << msg;
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


void Tray::generateMenu(bool init) {
  {
    if (NULL == trayMenu)
      trayMenu = new QMenu();
    else
      trayMenu->clear(); // Clear old menu

    if (!init){
      // Setup the new menu
      if (!libada::i()->isDeviceConnected()) {
        trayMenu->addAction(tr("Nitrokey not connected"));
      } else {
        if (!libada::i()->isStorageDeviceConnected()) // Nitrokey Pro connected
          generateMenuForProDevice();
        else {
          // Nitrokey Storage is connected
          generateMenuForStorageDevice();
        }
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
}

void Tray::initActionsForStick10() {
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

void Tray::initCommonActions() {
  DebugAction = new QAction(tr("&Debug"), this);
  connect(DebugAction, SIGNAL(triggered()), this, SLOT(startStickDebug()));

  quitAction = new QAction(tr("&Quit"), this);
  quitAction->setIcon(QIcon(":/images/quit.png"));
  connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

  ActionAboutDialog = new QAction(tr("&About Nitrokey"), this);

  ActionAboutDialog->setIcon(QIcon(":/images/about.png"));
  connect(ActionAboutDialog, SIGNAL(triggered()), this, SLOT(startAboutDialog()));
}

void Tray::initActionsForStick20() {
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


void Tray::generatePasswordMenu() {
  if (trayMenuPasswdSubMenu != NULL) {
    delete trayMenuPasswdSubMenu;
  }
  trayMenuPasswdSubMenu = new QMenu(tr("Passwords")); //TODO make shared pointer

  trayMenu->addMenu(trayMenuPasswdSubMenu);
  trayMenu->addSeparator();

  tray_Worker *worker = new tray_Worker;
  worker->moveToThread(&thread_tray_populateOTP);
//  connect(&tray_populateOTP, &QThread::finished, worker, &QObject::deleteLater);
  connect(&thread_tray_populateOTP, SIGNAL(started()), worker, SLOT(doWork()));
  connect(worker, SIGNAL(resultReady()), this, SLOT(populateOTPPasswordMenu()));
  //FIXME connect this to mainwindow
//  connect(worker, SIGNAL(resultReady()), this, SLOT(generateComboBoxEntrys()));

  thread_tray_populateOTP.start();
}

void Tray::populateOTPPasswordMenu() {
  for (int i=0; i < TOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getTOTPSlotName(i);
    if (!slotName.empty()){
      trayMenuPasswdSubMenu->addAction(QString::fromStdString(slotName),
                                       this, [=](){ getTOTPDialog(i);} );
    }
  }

  for (int i=0; i<HOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getHOTPSlotName(i);
    if (!slotName.empty()){
      trayMenuPasswdSubMenu->addAction(QString::fromStdString(slotName),
                                       this, [=](){ getHOTPDialog(i);} );
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
  if (trayMenuPasswdSubMenu->actions().empty()) {
    trayMenuPasswdSubMenu->hide();
  }
}


void Tray::generateMenuForProDevice() {
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
using nm = nitrokey::NitrokeyManager;

void Tray::generateMenuForStorageDevice() {
  int AddSeperator = FALSE;
  {
    auto status = nm::instance()->get_status_storage();


    if (status.ActiveSD_CardID_u32 == 0) // Is Stick 2.0 online (SD + SC
      // accessable?)
    {
      trayMenu->addAction(Stick20ActionUpdateStickStatus);
      return;
    }

    // Add special entrys
//    if (TRUE == StickNotInitated) {
    if (TRUE == status.StickKeysNotInitiated) {
      trayMenu->addAction(Stick20ActionInitCryptedVolume);
      AddSeperator = TRUE;
    }

//    if (FALSE == StickNotInitated && TRUE == SdCardNotErased) {
    if (!status.StickKeysNotInitiated && !status.SDFillWithRandomChars_u8) {
      trayMenu->addAction(Stick20ActionFillSDCardWithRandomChars);
      AddSeperator = TRUE;
    }

    if (TRUE == AddSeperator)
      trayMenu->addSeparator();

    generatePasswordMenu();
    trayMenu->addSeparator();

    if (!status.StickKeysNotInitiated) {
      // Enable tab for password safe for stick 2
      if (-1 == ui->tabWidget->indexOf(ui->tab_3)) {
        ui->tabWidget->addTab(ui->tab_3, tr("Password Safe"));
      }

      // Setup entrys for password safe
      generateMenuPasswordSafe();
    }

//    if (FALSE == SdCardNotErased) {
    if (!status.SDFillWithRandomChars_u8) {
//      if (FALSE == CryptedVolumeActive)
      if (status.VolumeActiceFlag_st.encrypted)
        trayMenu->addAction(Stick20ActionEnableCryptedVolume);
      else
        trayMenu->addAction(Stick20ActionDisableCryptedVolume);

//      if (FALSE == HiddenVolumeActive)
      if (status.VolumeActiceFlag_st.hidden)
        trayMenu->addAction(Stick20ActionEnableHiddenVolume);
      else
        trayMenu->addAction(Stick20ActionDisableHiddenVolume);
    }

//    if (FALSE != (HiddenVolumeActive || CryptedVolumeActive || PasswordSafeEnabled))
    if (FALSE != (status.VolumeActiceFlag_st.hidden || status.VolumeActiceFlag_st.encrypted || PasswordSafeEnabled))
      trayMenu->addAction(LockDeviceAction);

    trayMenuSubConfigure = trayMenu->addMenu(tr("Configure"));
    trayMenuSubConfigure->setIcon(QIcon(":/images/settings.png"));
    trayMenuSubConfigure->addAction(configureActionStick20);
    trayMenuSubConfigure->addSeparator();

    // Pin actions
    trayMenuSubConfigure->addAction(Stick20ActionChangeUserPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeAdminPIN);
    trayMenuSubConfigure->addAction(Stick20ActionChangeUpdatePIN);
    trayMenuSubConfigure->addSeparator();

    // Storage actions
    if (status.ReadWriteFlagUncryptedVolume_u8)
      trayMenuSubConfigure->addAction(Stick20ActionSetReadonlyUncryptedVolume); // Set
      // RW
      // active
    else
      trayMenuSubConfigure->addAction(Stick20ActionSetReadWriteUncryptedVolume); // Set
    // readonly
    // active

//    if (FALSE == SdCardNotErased)
    if (FALSE == status.SDFillWithRandomChars_u8)
      trayMenuSubConfigure->addAction(Stick20ActionSetupHiddenVolume);

    trayMenuSubConfigure->addAction(Stick20ActionDestroyCryptedVolume);
    trayMenuSubConfigure->addSeparator();

    // Other actions
    if (TRUE == LockHardware)
      trayMenuSubConfigure->addAction(Stick20ActionLockStickHardware);


    trayMenuSubConfigure->addAction(Stick20ActionEnableFirmwareUpdate);
    trayMenuSubConfigure->addAction(Stick20ActionExportFirmwareToFile);

    trayMenuSubConfigure->addSeparator();

    if (TRUE == ExtendedConfigActive) {
      trayMenuSubSpecialConfigure = trayMenuSubConfigure->addMenu(tr("Special Configure"));
      trayMenuSubSpecialConfigure->addAction(Stick20ActionFillSDCardWithRandomChars);

      if (TRUE == status.SDFillWithRandomChars_u8)
        trayMenuSubSpecialConfigure->addAction(Stick20ActionClearNewSDCardFound);
    }

    // Enable "reset user PIN" ?
    if (0 == libada::i()->getUserPasswordRetryCount()) {
      trayMenu->addSeparator();
      trayMenu->addAction(Stick20ActionResetUserPassword);
    }

//    // Add debug window ?
//    if (TRUE == DebugWindowActive) {
//      trayMenu->addSeparator();
//      trayMenu->addAction(Stick20ActionDebugAction);
//    }

    // Setup OTP combo box
    //  generateComboBoxEntrys();
  }
}

int Tray::UpdateDynamicMenuEntrys(void) {
//  auto m = nitrokey::NitrokeyManager::instance();
//  if (!m->is_connected()) return FALSE;
//  auto s = m->get_status_storage();
//
//  NormalVolumeRWActive =
//      s.ReadWriteFlagUncryptedVolume_u8 ? FALSE : TRUE;
//
//  CryptedVolumeActive =
//      s.VolumeActiceFlag_st.encrypted ? TRUE : FALSE;
//
//  HiddenVolumeActive =
//      s.VolumeActiceFlag_st.hidden ? TRUE : FALSE;
//
//  StickNotInitated = s.StickKeysNotInitiated ? TRUE : FALSE;
//
//  SdCardNotErased = s.SDFillWithRandomChars_u8 ? TRUE : FALSE;
//
//  Stick20ScSdCardOnline = TRUE;

  generateMenu(false);

  return (TRUE);
}

void Tray::generateMenuPasswordSafe() {
//  if (FALSE == cryptostick->passwordSafeUnlocked) {
//    {
//      trayMenu->addAction(UnlockPasswordSafeAction);
//
//      UnlockPasswordSafeAction->setEnabled(true == cryptostick->passwordSafeAvailable);
//    }
//  }
}




