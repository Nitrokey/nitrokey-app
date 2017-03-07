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
#include <libnitrokey/include/NitrokeyManager.h>
#include <QMenu>

Tray::Tray(QObject *_parent, bool _debug_mode, bool _extended_config,
           StorageActions *actions) :
    QObject(_parent), trayMenuPasswdSubMenu(nullptr), trayMenu(nullptr) {
  main_window = _parent;
  storageActions = actions;
  debug_mode = _debug_mode;
  ExtendedConfigActive = _extended_config;

  createIndicator();
  initActionsForStick10();
  initActionsForStick20();
  initCommonActions();

  generateMenu(true);
}

Tray::~Tray() {
  destroyThread();
}


/*
 * Create the tray menu.
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
  {
    static QMutex mtx;
    QMutexLocker locker(&mtx);

    if (nullptr == trayMenu)
      trayMenu = std::make_shared<QMenu>();
    else
      trayMenu->clear(); // Clear old menu

    run_before(trayMenu.get());

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
    if (debug_mode)
      trayMenu->addAction(DebugAction);

    trayMenu->addSeparator();

    // About entry
    trayMenu->addAction(ActionAboutDialog);

    trayMenu->addAction(quitAction);
    trayIcon->setContextMenu(trayMenu.get());
  }
}

void Tray::initActionsForStick10() {
  UnlockPasswordSafeAction = new QAction(tr("Unlock password safe"), main_window);
  UnlockPasswordSafeAction->setIcon(QIcon(":/images/safe.png"));
  connect(UnlockPasswordSafeAction, SIGNAL(triggered()), main_window, SLOT(PWS_Clicked_EnablePWSAccess()));

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

  quitAction = new QAction(tr("&Quit"), main_window);
  quitAction->setIcon(QIcon(":/images/quit.png"));
  connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

  ActionAboutDialog = new QAction(tr("&About Nitrokey"), main_window);

  ActionAboutDialog->setIcon(QIcon(":/images/about.png"));
  connect(ActionAboutDialog, SIGNAL(triggered()), main_window, SLOT(startAboutDialog()));
}

void Tray::initActionsForStick20() {
  configureActionStick20 = new QAction(tr("&OTP and Password safe"), main_window);
  connect(configureActionStick20, SIGNAL(triggered()), main_window, SLOT(startConfiguration()));

  Stick20ActionEnableCryptedVolume = new QAction(tr("&Unlock encrypted volume"), main_window);
  Stick20ActionEnableCryptedVolume->setIcon(QIcon(":/images/harddrive.png"));
  connect(Stick20ActionEnableCryptedVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20EnableCryptedVolume()));

  Stick20ActionDisableCryptedVolume = new QAction(tr("&Lock encrypted volume"), main_window);
  Stick20ActionDisableCryptedVolume->setIcon(QIcon(":/images/harddrive.png"));
  connect(Stick20ActionDisableCryptedVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20DisableCryptedVolume()));

  Stick20ActionEnableHiddenVolume = new QAction(tr("&Unlock hidden volume"), main_window);
  Stick20ActionEnableHiddenVolume->setIcon(QIcon(":/images/harddrive.png"));
  connect(Stick20ActionEnableHiddenVolume, SIGNAL(triggered()), storageActions,
          SLOT(startStick20EnableHiddenVolume()));

  Stick20ActionDisableHiddenVolume = new QAction(tr("&Lock hidden volume"), main_window);
  connect(Stick20ActionDisableHiddenVolume, SIGNAL(triggered()), storageActions,
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
  connect(LockDeviceAction, SIGNAL(triggered()), main_window, SLOT(startLockDeviceAction()));

  Stick20ActionUpdateStickStatus = new QAction(tr("Smartcard or SD card are not ready"), main_window);
  connect(Stick20ActionUpdateStickStatus, SIGNAL(triggered()), main_window, SLOT(startAboutDialog()));
}


std::shared_ptr<QThread> thread_tray_populateOTP;

void tray_Worker::doWork() {
  auto passwordSafeUnlocked = libada::i()->isPasswordSafeUnlocked();
  const auto total = TOTP_SLOT_COUNT+HOTP_SLOT_COUNT+
      (passwordSafeUnlocked?PWS_SLOT_COUNT:0)+1;
  int p = 0;
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

void Tray::generatePasswordMenu() {
  trayMenuPasswdSubMenu = std::make_shared<QMenu>(tr("Passwords")); //TODO make shared pointer

  trayMenu->addMenu(trayMenuPasswdSubMenu.get());
  trayMenu->addSeparator();

  if (thread_tray_populateOTP!= nullptr){
    destroyThread();
  }
  thread_tray_populateOTP = std::make_shared<QThread>();
  tray_Worker *worker = new tray_Worker;
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
  for (int i=0; i < TOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getTOTPSlotName(i);
    bool slotProgrammed = libada::i()->isTOTPSlotProgrammed(i);
    if (slotProgrammed){
      trayMenuPasswdSubMenu->addAction(QString::fromStdString(slotName),
                                       this, [=](){
              //FIXME change in final code to static_cast
              dynamic_cast<MainWindow *>(main_window)->getTOTPDialog(i);
          } );
    }
  }

  for (int i=0; i<HOTP_SLOT_COUNT; i++){
    auto slotName = libada::i()->getHOTPSlotName(i);
    bool slotProgrammed = libada::i()->isHOTPSlotProgrammed(i);
    if (slotProgrammed){
      trayMenuPasswdSubMenu->addAction(QString::fromStdString(slotName),
                                       this, [=](){
              //FIXME change in final code to static_cast
              dynamic_cast<MainWindow *>(main_window)->getHOTPDialog(i);
          } );
    }
  }

  if (TRUE == libada::i()->isPasswordSafeUnlocked()) {
    for (int i = 0; i < PWS_SLOT_COUNT; i++) {
      if (libada::i()->getPWSSlotStatus(i)) {
        trayMenuPasswdSubMenu->addAction(QString::fromStdString(libada::i()->getPWSSlotName(i)),
                                         this, [=]() {
                //FIXME change in final code to static_cast
                dynamic_cast<MainWindow *>(main_window)->PWS_ExceClickedSlot(i);
            });
      }
    }
  }
  if (trayMenuPasswdSubMenu->actions().empty()) {
    trayMenuPasswdSubMenu->hide();
    trayMenuPasswdSubMenu->setEnabled(false);
    trayMenuPasswdSubMenu->setTitle( trayMenuPasswdSubMenu->title() + " " + tr("(empty)") );
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
    nitrokey::proto::stick20::DeviceConfigurationResponsePacket::ResponsePayload status;

    try {
      status = nm::instance()->get_status_storage();
    }
    catch (LongOperationInProgressException &e){
      return;
    }
    catch (DeviceCommunicationException &e){
      //TODO add info to tray about the error?
      return;
    }



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
      //FIXME move to mainwindow
      // Enable tab for password safe for stick 2
//      if (-1 == ui->tabWidget->indexOf(ui->tab_3)) {
//        ui->tabWidget->addTab(ui->tab_3, tr("Password Safe"));
//      }

      // Setup entrys for password safe
      generateMenuPasswordSafe();
    }

//    if (FALSE == SdCardNotErased) {
    if (status.SDFillWithRandomChars_u8) { //filled randomly
      if (!status.VolumeActiceFlag_st.encrypted)
        trayMenu->addAction(Stick20ActionEnableCryptedVolume);
      else
        trayMenu->addAction(Stick20ActionDisableCryptedVolume);

      if (!status.VolumeActiceFlag_st.hidden)
        trayMenu->addAction(Stick20ActionEnableHiddenVolume);
      else
        trayMenu->addAction(Stick20ActionDisableHiddenVolume);
    }

    //FIXME run in separate thread
    const auto PasswordSafeEnabled = libada::i()->isPasswordSafeUnlocked();
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
    auto read_write_active = status.ReadWriteFlagUncryptedVolume_u8 == 0;
    if (read_write_active)
      // Set readonly active
      trayMenuSubConfigure->addAction(Stick20ActionSetReadonlyUncryptedVolume);
    else
      // Set RW active
      trayMenuSubConfigure->addAction(Stick20ActionSetReadWriteUncryptedVolume);

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

    trayMenuSubConfigure->addSeparator();

    if (TRUE == ExtendedConfigActive) {
      trayMenuSubSpecialConfigure = trayMenuSubConfigure->addMenu(tr("Special Configure"));
      trayMenuSubSpecialConfigure->addAction(Stick20ActionFillSDCardWithRandomChars);

      if (status.NewSDCardFound_u8 && !status.SDFillWithRandomChars_u8)
        trayMenuSubSpecialConfigure->addAction(Stick20ActionClearNewSDCardFound);

      trayMenuSubSpecialConfigure->addAction(resetAction);
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

  }
}

int Tray::UpdateDynamicMenuEntrys(void) {
  generateMenu(false);
  return (TRUE);
}

void Tray::generateMenuPasswordSafe() {
  auto passwordSafeUnlocked = libada::i()->isPasswordSafeUnlocked();
  if (!passwordSafeUnlocked) {
      trayMenu->addAction(UnlockPasswordSafeAction);

      auto passwordSafeAvailable = libada::i()->isPasswordSafeAvailable();
      UnlockPasswordSafeAction->setEnabled(passwordSafeAvailable);
  } else {
    trayMenu->addAction(LockDeviceAction);
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
  static bool done = false;
  if (!done){
    this->showTrayMessage(te);
    trayMenu->addAction(a.get());
    done = true;
  }
  a->setText(te);
  trayIcon->setContextMenu(trayMenu.get());
}

