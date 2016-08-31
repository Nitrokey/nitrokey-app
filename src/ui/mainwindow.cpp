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

#include "mainwindow.h"
#include "aboutdialog.h"
#include "base32.h"
#include "device.h"
#include "hotpdialog.h"
#include "passworddialog.h"
#include "pindialog.h"
#include "response.h"
#include "sleep.h"
#include "stick20debugdialog.h"
#include "stick20dialog.h"
#include "string.h"
#include "ui_mainwindow.h"
#include <stdio.h>
#include <string.h>

#include "device.h"
#include "mcvs-wrapper.h"
#include "nitrokey-applet.h"
#include "passwordsafedialog.h"
#include "response.h"
#include "securitydialog.h"
#include "stick20-response-task.h"
#include "stick20changepassworddialog.h"
#include "stick20hiddenvolumedialog.h"
#include "stick20infodialog.h"
#include "stick20lockfirmwaredialog.h"
#include "stick20matrixpassworddialog.h"
#include "stick20responsedialog.h"
#include "stick20setup.h"
#include "stick20updatedialog.h"

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
  if (DebugingActive == TRUE)
    qDebug() << "Unmounting " << mntdir.c_str();
  // TODO polling with MNT_EXPIRE? test which will suit better
  // int err = umount2("/dev/nitrospace", MNT_DETACH);
  int err = umount(mntdir.c_str());
  if (err != 0) {
    if (DebugingActive == TRUE)
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

#ifdef HAVE_LIBAPPINDICATOR
/*
 * Indicator call backs
 */
extern "C" {
void onQuit(GtkMenu *, gpointer);
void onAbout(GtkMenu *, gpointer);
void onChangeUserPin(GtkMenu *, gpointer);
void onChangeAdminPin(GtkMenu *, gpointer);
void onChangeUpdatePin(GtkMenu *, gpointer);
void onResetUserPin(GtkMenu *, gpointer);
void onConfigure(GtkMenu *, gpointer);
void onEnablePasswordSafe(GtkMenu *, gpointer);
void onReset(GtkMenu *, gpointer);
void onGetTOTP(GtkMenu *, gpointer);
void onGetHOTP(GtkMenu *, gpointer);
void onGetPasswordSafeSlot(GtkMenu *, gpointer);
void onInitEncryptedVolume(GtkMenu *, gpointer);
void onFillSDCardWithRandomChars(GtkMenu *, gpointer);
void onEnableEncryptedVolume(GtkMenu *, gpointer);
void onDisableEncryptedVolume(GtkMenu *, gpointer);
void onEnableHiddenVolume(GtkMenu *, gpointer);
void onDisableHiddenVolume(GtkMenu *, gpointer);
void onSetupHiddenVolumeItem(GtkMenu *, gpointer);
void onLockDevice(GtkMenu *, gpointer);
void onSetReadOnlyUnencryptedVolumeItem(GtkMenu *, gpointer);
void onSetReadWriteUnencryptedVolumeItem(GtkMenu *, gpointer);
void onChangeUserPinStorage(GtkMenu *, gpointer);
void onChangeAdminPinStorage(GtkMenu *, gpointer);
void onLockHardware(GtkMenu *, gpointer);
void onEnableFirmwareUpdate(GtkMenu *, gpointer);
void onExportFirmwareToFile(GtkMenu *, gpointer);
void onResetUserPassword(GtkMenu *, gpointer);
void onDebug(GtkMenu *, gpointer);
void onClearNewSDCardFound(GtkMenu *, gpointer);
void onSetupHiddenVolume(GtkMenu *, gpointer);
void onSetupPasswordMatrix(GtkMenu *, gpointer);
bool isUnity(void);
}

void onQuit(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  QApplication *self = static_cast<QApplication *>(data);
  self->quit();
}

void onAbout(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startAboutDialog();
}

void onChangeUserPin(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick10ActionChangeUserPIN();
}

void onChangeAdminPin(GtkMenu *menu, gpointer data) {

  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick10ActionChangeAdminPIN();
}

void onChangeUpdatePin(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20ActionChangeUpdatePIN();
}

void onResetUserPin(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startResetUserPassword();
}

void onConfigure(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startConfiguration();
}

void onEnablePasswordSafe(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->PWS_Clicked_EnablePWSAccess();
}

void onReset(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->factoryReset();
}

struct getOTPData {
  MainWindow *window;
  int slot;
};

void onGetTOTP(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  struct getOTPData *otp_data = static_cast<struct getOTPData *>(data);
  otp_data->window->getTOTPDialog(otp_data->slot);
}

void onGetHOTP(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  struct getOTPData *otp_data = static_cast<struct getOTPData *>(data);
  otp_data->window->getHOTPDialog(otp_data->slot);
}

void onGetPasswordSafeSlot(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  struct getOTPData *otp_data = static_cast<struct getOTPData *>(data);
  otp_data->window->PWS_ExceClickedSlot(otp_data->slot);
}

void onInitEncryptedVolume(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20DestroyCryptedVolume(1);
}

void onFillSDCardWithRandomChars(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20FillSDCardWithRandomChars();
}

void onEnableEncryptedVolume(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20EnableCryptedVolume();
}

void onDisableEncryptedVolume(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20DisableCryptedVolume();
}

void onEnableHiddenVolume(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20EnableHiddenVolume();
}

void onDisableHiddenVolume(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20DisableHiddenVolume();
}

void onSetupHiddenVolumeItem(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20SetupHiddenVolume();
}

void onLockDevice(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startLockDeviceAction();
}

void onChangeUserPinStorage(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20ActionChangeUserPIN();
}

void onChangeAdminPinStorage(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20ActionChangeAdminPIN();
}

void onSetReadOnlyUnencryptedVolumeItem(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20SetReadOnlyUncryptedVolume();
}

void onSetReadWriteUnencryptedVolumeItem(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20SetReadWriteUncryptedVolume();
}

void onLockHardware(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20LockStickHardware();
}

void onEnableFirmwareUpdate(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20EnableFirmwareUpdate();
}

void onExportFirmwareToFile(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20ExportFirmwareToFile();
}

void onResetUserPassword(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startResetUserPassword();
}

void onDebug(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20DebugAction();
}

void onClearNewSDCardFound(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20ClearNewSdCardFound();
}

void onSetupHiddenVolume(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20SetupHiddenVolume();
}

void onSetupPasswordMatrix(GtkMenu *menu, gpointer data) {
  Q_UNUSED(menu);
  MainWindow *window = static_cast<MainWindow *>(data);
  window->startStick20SetupPasswordMatrix();
}

bool isUnity() {
  QString desktop = getenv("XDG_CURRENT_DESKTOP");

  return (desktop.toLower() == "unity" || desktop.toLower() == "kde" ||
          desktop.toLower() == "lxde" || desktop.toLower() == "xfce");
}

#endif // HAVE_LIBAPPINDICATOR

void MainWindow::showTrayMessage(const QString &title, const QString &msg,
                                 enum trayMessageType type, int timeout) {
#ifdef HAVE_LIBAPPINDICATOR
  if (isUnity()) {
    if (!notify_init("example"))
      return;

    NotifyNotification *notf;

    notf = notify_notification_new(title.toUtf8().data(), msg.toUtf8().data(), NULL);
    notify_notification_show(notf, NULL);
    notify_uninit();
  } else
#endif // HAVE_LIBAPPINDICATOR
  {
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
      csApplet->messageBox(msg);
  }
}

/*
 * Create the tray menu.
 * In Unity we create an AppIndicator
 * In all other systems we use Qt's tray
 */
void MainWindow::createIndicator() {
#ifdef HAVE_LIBAPPINDICATOR
  if (isUnity()) {
    indicator = app_indicator_new("Nitrokey App", "nitrokey-app", APP_INDICATOR_CATEGORY_OTHER);
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
  } else // other DE's and OS's
#endif   // HAVE_LIBAPPINDICATOR
  {
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/images/CS_icon.png"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this,
            SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    trayIcon->show();
  }

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
  int ret;

  QMetaObject::Connection ret_connection;

  InitState();
  clipboard = QApplication::clipboard();
  ExtendedConfigActive = StartupInfo_st->ExtendedConfigActive;

  if (0 != StartupInfo_st->PasswordMatrix)
    MatrixInputActive = TRUE;

  if (0 != StartupInfo_st->LockHardware)
    LockHardware = TRUE;

  switch (StartupInfo_st->FlagDebug) {
  case DEBUG_STATUS_LOCAL_DEBUG:
    DebugWindowActive = TRUE;
    DebugingActive = TRUE;
    DebugingStick20PoolingActive = FALSE;
    break;

  case DEBUG_STATUS_DEBUG_ALL:
    DebugWindowActive = TRUE;
    DebugingActive = TRUE;
    DebugingStick20PoolingActive = TRUE;
    break;

  case DEBUG_STATUS_NO_DEBUGGING:
  default:
    DebugWindowActive = FALSE;
    DebugingActive = FALSE;
    DebugingStick20PoolingActive = FALSE;
    break;
  }

  ui->setupUi(this);
  ui->tabWidget->setCurrentIndex(0); // Set first tab active
  ui->PWS_ButtonCreatePW->setText(QString(tr("Generate random password ")));
  ui->statusBar->showMessage(tr("Nitrokey disconnected"));
  cryptostick = new Device(VID_STICK_OTP, PID_STICK_OTP, VID_STICK_20, PID_STICK_20,
                           VID_STICK_20_UPDATE_MODE, PID_STICK_20_UPDATE_MODE);

  // Check for comamd line execution after init "nitrokey"
  if (0 != StartupInfo_st->Cmd) {
    initDebugging();
    ret = ExecStickCmd(StartupInfo_st->CmdLine);
    exit(ret);
  }

  set_initial_time = false;
  QTimer *timer = new QTimer(this);

  ret_connection = connect(timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
  timer->start(1000);

  QTimer *Clipboard_ValidTimer = new QTimer(this);

  // Start timer for Clipboard delete check
  connect(Clipboard_ValidTimer, SIGNAL(timeout()), this, SLOT(checkClipboard_Valid()));
  Clipboard_ValidTimer->start(1000);

  QTimer *Password_ValidTimer = new QTimer(this);

  // Start timer for Password check
  connect(Password_ValidTimer, SIGNAL(timeout()), this, SLOT(checkPasswordTime_Valid()));
  Password_ValidTimer->start(1000);

  createIndicator();

  if (FALSE == DebugWindowActive) {
    ui->frame_6->setVisible(false);
    ui->testHOTPButton->setVisible(false);
    ui->testTOTPButton->setVisible(false);
    ui->testsSpinBox->setVisible(false);
    ui->testsSpinBox_2->setVisible(false);
    ui->testsLabel->setVisible(false);
    ui->testsLabel_2->setVisible(false);
  }

  initActionsForStick10();
  initActionsForStick20();
  initCommonActions();

  connect(ui->secretEdit, SIGNAL(textEdited(QString)), this, SLOT(checkTextEdited()));

  // Init debug text
  initDebugging();

  ui->deleteUserPasswordCheckBox->setEnabled(false);
  ui->deleteUserPasswordCheckBox->setChecked(false);

  cryptostick->getStatus();
  generateMenu();
}

#define MAX_CONNECT_WAIT_TIME_IN_SEC 10

int MainWindow::ExecStickCmd(char *Cmdline) {
  int i;
  char *p;
  bool ret;

  printf("Connecting to nitrokey");
  // Wait for connect
  for (i = 0; i < 2 * MAX_CONNECT_WAIT_TIME_IN_SEC; i++) {
    if (cryptostick->isConnected == true) {
      break;
    } else {
      cryptostick->connect();
      OwnSleep::msleep(500);
      cryptostick->checkConnection(FALSE);
      printf(".");
      fflush(stdout);
    }
  }
  if (MAX_CONNECT_WAIT_TIME_IN_SEC <= i) {
    printf("ERROR: Can't get connection to nitrokey\n");
    return (1);
  }
  // Check device
  printf(" connected. \n");

  // Get command
  p = strchr(Cmdline, '=');
  if (NULL == p) {
    p = NULL;
  } else {
    *p = 0; // set end of string in place of '=' sign
    p++;    // Points now to 1. parameter of command
  }

  // Check password and set default for empty
  if (p == NULL || 0 == strlen(p)) {
    p = (char *)"12345678";
    //   FIXME should issue warning instead of silent password assigning?
  }
  uint8_t password[40] = {0};
  password[0] = 'p'; // Send a clear password
  STRCPY((char *)&password[1], sizeof(password) - 1, p);

  // --cmd factoryReset=<ADMIN PIN>
  if (0 == strcmp(Cmdline, "factoryReset")) {
    // this command requires clear password without prefix
    ret = cryptostick->factoryReset(p);
    printf("%s\n", getFactoryResetMessage(ret));
    cryptostick->disconnect();
    return ret;
  }
  // --cmd setUpdateMode=<FIRMWARE PASSWORD>
  else if (0 == strcmp(Cmdline, "setUpdateMode")) {
    ret = cryptostick->stick20EnableFirmwareUpdate(password);
    if (false == ret) {
      printf("Command execution has failed or device stopped responding.\n");
      cryptostick->disconnect();
      return (1);
    }
  }
  //  usage:
  // --cmd unlockencrypted=<user_pin>
  //  example:
  // --cmd unlockencrypted=123456
  else if (0 == strncmp(Cmdline, "unlockencrypted", strlen("unlockencrypted"))) {
    printf("Unlock encrypted volume ");
    ret = cryptostick->stick20EnableCryptedPartition(password);

    if (false == ret) {
      printf("FAIL sending command via HID\n");
    } else {
      printf("success\n");
    }
    cryptostick->disconnect();
    return (ret);
    //  usage:
    // --cmd prodinfo
  } else if (0 == strcmp(Cmdline, "prodinfo")) {
    printf("Send -get production infos-\n");

    bool commandSuccess = cryptostick->stick20ProductionTest();
    if (!commandSuccess) {
      printf("Command execution has failed or device stopped responding.\n");
    }

    if (TRUE == Stick20_ProductionInfosChanged) {
      Stick20_ProductionInfosChanged = FALSE;
      if (FALSE == AnalyseProductionInfos()) {
        cryptostick->disconnect();
        return (1);
      }
    } else {
      cryptostick->disconnect();
      return (1);
    }
  } else {
    printf("Unknown command\n");
  }
  cryptostick->disconnect();
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
  char text[100];

  bool ProductStateOK = TRUE;

  printf("\nGet production infos\n");

  printf("Firmware     %d.%d - %d\n",
         (unsigned int)Stick20ProductionInfos_st.FirmwareVersion_au8[0],
         (unsigned int)Stick20ProductionInfos_st.FirmwareVersion_au8[1],
         (unsigned int)Stick20ProductionInfos_st.FirmwareVersionInternal_u8);
  printf("CPU ID       0x%08x\n", Stick20ProductionInfos_st.CPU_CardID_u32);
  printf("Smartcard ID 0x%08x\n", Stick20ProductionInfos_st.SmartCardID_u32);
  printf("SD card ID   0x%08x\n", Stick20ProductionInfos_st.SD_CardID_u32);

  printf((char *)"\nSD card infos\n");
  printf("Manufacturer 0x%02x\n", Stick20ProductionInfos_st.SD_Card_Manufacturer_u8);
  printf("OEM          0x%04x\n", Stick20ProductionInfos_st.SD_Card_OEM_u16);
  printf("Manufa. date %d.%02d\n", Stick20ProductionInfos_st.SD_Card_ManufacturingYear_u8 + 2000,
         Stick20ProductionInfos_st.SD_Card_ManufacturingMonth_u8);
  printf("Size         %2d GB\n", Stick20ProductionInfos_st.SD_Card_Size_u8);

  // Valid manufacturers
  if ((0x74 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8)    // Transcend
      && (0x03 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8) // SanDisk
      && (0x73 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8) // ?
      // Amazon
      && (0x28 != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8) // Lexar
      && (0x1b != Stick20ProductionInfos_st.SD_Card_Manufacturer_u8) // Samsung
      ) {
    ProductStateOK = FALSE;
    printf((char *)"Manufacturers error\n");
  }

  // Write to log
  {
    FILE *fp;

    char *LogFile = (char *)"prodlog.txt";

    FOPEN(fp, LogFile, "a+");
    if (0 != fp) {
      QDateTime ActualTimeStamp(QDateTime::currentDateTime());
      std::string timestr = ActualTimeStamp.toString("dd.MM.yyyy hh:mm:ss").toStdString();
      const char *timestamp = timestr.c_str();
      fprintf(fp, "timestamp:%s,", timestamp);
      fprintf(fp, "CPU:0x%08x,", Stick20ProductionInfos_st.CPU_CardID_u32);
      fprintf(fp, "SC:0x%08x,", Stick20ProductionInfos_st.SmartCardID_u32);
      fprintf(fp, "SD:0x%08x,", Stick20ProductionInfos_st.SD_CardID_u32);
      fprintf(fp, "SCM:0x%02x,", Stick20ProductionInfos_st.SD_Card_Manufacturer_u8);
      fprintf(fp, "SCO:0x%04x,", Stick20ProductionInfos_st.SD_Card_OEM_u16);
      fprintf(fp, "DAT:%d.%02d,", Stick20ProductionInfos_st.SD_Card_ManufacturingYear_u8 + 2000,
              Stick20ProductionInfos_st.SD_Card_ManufacturingMonth_u8);
      fprintf(fp, "Size:%d,", Stick20ProductionInfos_st.SD_Card_Size_u8);
      fprintf(fp, "Firmware:%d.%d - %d",
              (unsigned int)Stick20ProductionInfos_st.FirmwareVersion_au8[0],
              (unsigned int)Stick20ProductionInfos_st.FirmwareVersion_au8[1],
              (unsigned int)Stick20ProductionInfos_st.FirmwareVersionInternal_u8);
      if (FALSE == ProductStateOK) {
        fprintf(fp, ",*** FAIL");
      }
      fprintf(fp, "\n");
      fclose(fp);
    } else {
      printf((char *)"\n*** CAN'T WRITE TO LOG FILE -%s-***\n", LogFile);
    }
  }

  if (TRUE == ProductStateOK) {
    printf((char *)"\nStick OK\n");
  } else {
    printf((char *)"\n**** Stick NOT OK ****\n");
  }

  DebugAppendTextGui((char *)"Production Infos\n");

  SNPRINTF(text, sizeof(text), "Firmware     %d.%d\n",
           (unsigned int)Stick20ProductionInfos_st.FirmwareVersion_au8[0],
           (unsigned int)Stick20ProductionInfos_st.FirmwareVersion_au8[1]);
  DebugAppendTextGui(text);
  SNPRINTF(text, sizeof(text), "CPU ID       0x%08x\n", Stick20ProductionInfos_st.CPU_CardID_u32);
  DebugAppendTextGui(text);
  SNPRINTF(text, sizeof(text), "Smartcard ID 0x%08x\n", Stick20ProductionInfos_st.SmartCardID_u32);
  DebugAppendTextGui(text);
  SNPRINTF(text, sizeof(text), "SD card ID   0x%08x\n", Stick20ProductionInfos_st.SD_CardID_u32);
  DebugAppendTextGui(text);

  DebugAppendTextGui("Password retry count\n");
  SNPRINTF(text, sizeof(text), "Admin        %d\n", Stick20ProductionInfos_st.SC_AdminPwRetryCount);
  DebugAppendTextGui(text);
  SNPRINTF(text, sizeof(text), "User         %d\n", Stick20ProductionInfos_st.SC_UserPwRetryCount);
  DebugAppendTextGui(text);

  DebugAppendTextGui("SD card infos\n");
  SNPRINTF(text, sizeof(text), "Manufacturer 0x%02x\n",
           Stick20ProductionInfos_st.SD_Card_Manufacturer_u8);
  DebugAppendTextGui(text);
  SNPRINTF(text, sizeof(text), "OEM          0x%04x\n", Stick20ProductionInfos_st.SD_Card_OEM_u16);
  DebugAppendTextGui(text);
  SNPRINTF(text, sizeof(text), "Manufa. date %d.%02d\n",
           Stick20ProductionInfos_st.SD_Card_ManufacturingYear_u8 + 2000,
           Stick20ProductionInfos_st.SD_Card_ManufacturingMonth_u8);
  DebugAppendTextGui(text);
  SNPRINTF(text, sizeof(text), "Write speed  %d kB/sec\n",
           Stick20ProductionInfos_st.SD_WriteSpeed_u16);
  DebugAppendTextGui(text);
  SNPRINTF(text, sizeof(text), "Size         %d GB\n", Stick20ProductionInfos_st.SD_Card_Size_u8);
  DebugAppendTextGui(text);

  return (ProductStateOK);
}

void MainWindow::checkConnection() {
  static int DeviceOffline = TRUE;

  int ret = 0;

  currentTime = QDateTime::currentDateTime().toTime_t();

  int result = cryptostick->checkConnection(TRUE);

  // Set new slot counts
  HOTP_SlotCount = cryptostick->HOTP_SlotCount;
  TOTP_SlotCount = cryptostick->TOTP_SlotCount;

  if (result == 0) {
    if (false == cryptostick->activStick20) {
      ui->statusBar->showMessage(tr("Nitrokey Pro connected"));

      if (set_initial_time == FALSE) {
        ret = cryptostick->setTime(TOTP_CHECK_TIME);
        set_initial_time = TRUE;
      } else {
        ret = 0;
      }

      bool answer;

      if (ret == -2) {
        answer = csApplet->detailedYesOrNoBox(
            tr("Time is out-of-sync"),
            tr("WARNING!\n\nThe time of your computer and Nitrokey are out of "
               "sync. Your computer may be configured with a wrong time or "
               "your Nitrokey may have been attacked. If an attacker or "
               "malware could have used your Nitrokey you should reset the "
               "secrets of your configured One Time Passwords. If your "
               "computer's time is wrong, please configure it correctly and "
               "reset the time of your Nitrokey.\n\nReset Nitrokey's time?"),
            0, false);
        if (answer) {
          resetTime();
          QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
          Sleep::msleep(1000);
          QApplication::restoreOverrideCursor();
          generateAllConfigs();

          csApplet->messageBox(tr("Time reset!"));
        }
      }

      cryptostick->getStatus();
    } else
      ui->statusBar->showMessage(tr("Nitrokey Storage connected"));
    DeviceOffline = FALSE;
  } else if (result == -1) {
    ui->statusBar->showMessage(tr("Nitrokey disconnected"));
    HID_Stick20Init(); // Clear stick 20 data
    Stick20ScSdCardOnline = FALSE;
    CryptedVolumeActive = FALSE;
    HiddenVolumeActive = FALSE;
    set_initial_time = FALSE;
    if (FALSE == DeviceOffline) // To avoid the continuous reseting of
                                // the menu
    {
      generateMenu();
      DeviceOffline = TRUE;
      cryptostick->passwordSafeAvailable = true;
      showTrayMessage(tr("Nitrokey disconnected"), "", INFORMATION, TRAY_MSG_TIMEOUT);
    }
    cryptostick->connect();
  } else if (result == 1) { // recreate the settings and menus
    if (false == cryptostick->activStick20) {
      ui->statusBar->showMessage(tr("Nitrokey connected"));
      showTrayMessage(tr("Nitrokey connected"), "Nitrokey Pro", INFORMATION, TRAY_MSG_TIMEOUT);

      if (set_initial_time == FALSE) {
        ret = cryptostick->setTime(TOTP_CHECK_TIME);
        set_initial_time = TRUE;
      } else {
        ret = 0;
      }

      bool answer;

      if (ret == -2) {
        answer = csApplet->detailedYesOrNoBox(
            tr("Time is out-of-sync"),
            tr("WARNING!\n\nThe time of your computer and Nitrokey are out of "
               "sync. Your computer may be configured with a wrong time or "
               "your Nitrokey may have been attacked. If an attacker or "
               "malware could have used your Nitrokey you should reset the "
               "secrets of your configured One Time Passwords. If your "
               "computer's time is wrong, please configure it correctly and "
               "reset the time of your Nitrokey.\n\nReset Nitrokey's time?"),
            0, false);

        if (answer) {
          resetTime();
          QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
          Sleep::msleep(1000);
          QApplication::restoreOverrideCursor();
          generateAllConfigs();

          csApplet->messageBox(tr("Time reset!"));
        }
      }

      cryptostick->getStatus();
    } else {
      showTrayMessage(tr("Nitrokey connected"), "Nitrokey Storage", INFORMATION, TRAY_MSG_TIMEOUT);
      ui->statusBar->showMessage(tr("Nitrokey Storage connected"));
    }
    generateMenu();
  }

  // Be sure that the retry counter are always up to date
  if ((cryptostick->userPasswordRetryCount != HID_Stick20Configuration_st.UserPwRetryCount)) // (99
                                                                                             // !=
  // HID_Stick20Configuration_st.UserPwRetryCount)
  // &&
  {
    cryptostick->userPasswordRetryCount = HID_Stick20Configuration_st.UserPwRetryCount;
    cryptostick->passwordRetryCount = HID_Stick20Configuration_st.AdminPwRetryCount;
  }

  if (TRUE == Stick20_ConfigurationChanged && cryptostick->activStick20) {
    Stick20_ConfigurationChanged = FALSE;

    UpdateDynamicMenuEntrys();

    if (TRUE == StickNotInitated) {
      if (FALSE == StickNotInitated_DontAsk)
        csApplet->warningBox(tr("Warning: Encrypted volume is not secure,\nSelect \"Initialize "
                                "device\" option from context menu."));
    }
    if (FALSE == StickNotInitated && TRUE == SdCardNotErased) {
      if (FALSE == SdCardNotErased_DontAsk)
        csApplet->warningBox(tr("Warning: Encrypted volume is not secure,\nSelect \"Initialize "
                                "storage with random data\""));
    }
  }
  /*
      if (TRUE == Stick20_ProductionInfosChanged)
      {
          Stick20_ProductionInfosChanged = FALSE;
          AnalyseProductionInfos ();
      }
  */
  if (ret) {
  } // Fix warnings
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
    if ((char)cryptostick->TOTPSlots[i]->slotName[0] == '\0')
      ui->slotComboBox->addItem(QString(tr("TOTP slot ")).append(QString::number(i + 1, 10)));
    else
      ui->slotComboBox->addItem(QString(tr("TOTP slot "))
                                    .append(QString::number(i + 1, 10))
                                    .append(" [")
                                    .append((char *)cryptostick->TOTPSlots[i]->slotName)
                                    .append("]"));
  }

  ui->slotComboBox->insertSeparator(TOTP_SlotCount + 1);

  for (i = 0; i < HOTP_SlotCount; i++) {
    if ((char)cryptostick->HOTPSlots[i]->slotName[0] == '\0')
      ui->slotComboBox->addItem(QString(tr("HOTP slot ")).append(QString::number(i + 1, 10)));
    else
      ui->slotComboBox->addItem(QString(tr("HOTP slot "))
                                    .append(QString::number(i + 1, 10))
                                    .append(" [")
                                    .append((char *)cryptostick->HOTPSlots[i]->slotName)
                                    .append("]"));
  }

  ui->slotComboBox->setCurrentIndex(0);
}

void MainWindow::generateMenu() {
#ifdef HAVE_LIBAPPINDICATOR
  if (isUnity()) {
    indicatorMenu = gtk_menu_new();
    GtkWidget *notConnItem = gtk_menu_item_new_with_label(_("Nitrokey not connected"));
    GtkWidget *debugItem;

    GtkWidget *aboutItem = gtk_image_menu_item_new_with_label(_("About"));
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(aboutItem), TRUE);
    GtkWidget *aboutItemImg = gtk_image_new_from_file("/usr/share/nitrokey/info.png");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(aboutItem), GTK_WIDGET(aboutItemImg));

    GtkWidget *quitItem = gtk_image_menu_item_new_with_label(_("Quit"));
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(quitItem), TRUE);
    GtkWidget *quitItemImg = gtk_image_new_from_file("/usr/share/nitrokey/quit.png");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(quitItem), GTK_WIDGET(quitItemImg));

    // gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(quitItem),
    // quitItemImg);

    GtkWidget *separItem = gtk_separator_menu_item_new();

    g_signal_connect(aboutItem, "activate", G_CALLBACK(onAbout), this);
    g_signal_connect(quitItem, "activate", G_CALLBACK(onQuit), qApp);

    if (cryptostick->isConnected == false) {
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), notConnItem);
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), separItem);
    } else {
      if (false == cryptostick->activStick20)
        generateMenuForProDevice();
      else
        generateMenuForStorageDevice();
    }

    if (TRUE == DebugWindowActive) {
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), aboutItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), quitItem);

    gtk_widget_show(notConnItem);
    gtk_widget_show(separItem);
    gtk_widget_show(aboutItem);
    gtk_widget_show(quitItem);

    app_indicator_set_menu(indicator, GTK_MENU(indicatorMenu));
  } else
#endif // HAVE_LIBAPPINDICATOR
  {
    if (NULL == trayMenu)
      trayMenu = new QMenu();
    else
      trayMenu->clear(); // Clear old menu

    // Setup the new menu
    if (cryptostick->isConnected == false) {
      trayMenu->addAction(tr("Nitrokey not connected"));
      cryptostick->passwordSafeAvailable = true;
    } else {
      if (false == cryptostick->activStick20) // Nitrokey Pro connected
        generateMenuForProDevice();
      else {
        // Nitrokey Storage is connected
        generateMenuForStorageDevice();
        cryptostick->passwordSafeAvailable = true;
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

  SecPasswordAction = new QAction(tr("&SecPassword"), this);
  connect(SecPasswordAction, SIGNAL(triggered()), this, SLOT(startMatrixPasswordDialog()));

  Stick20SetupAction = new QAction(tr("&Stick 20 Setup"), this);
  connect(Stick20SetupAction, SIGNAL(triggered()), this, SLOT(startStick20Setup()));

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

  Stick20ActionGetStickStatus = new QAction(tr("&Get stick status"), this);
  connect(Stick20ActionGetStickStatus, SIGNAL(triggered()), this,
          SLOT(startStick20GetStickStatus()));

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
#ifdef HAVE_LIBAPPINDICATOR
  if (isUnity()) {
    GtkWidget *passwordsItem = gtk_menu_item_new_with_label(_("Passwords"));

    GtkWidget *separItem1 = gtk_separator_menu_item_new();

    GtkWidget *passwordsSubMenu = gtk_menu_new();

    gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), passwordsItem);
    for (int i = 0; i < TOTP_SlotCount; i++) {
      GtkWidget *currPasswdItem;

      struct getOTPData *otp_data;

      if (cryptostick->TOTPSlots[i]->isProgrammed) {
        otp_data = (struct getOTPData *)malloc(sizeof(struct getOTPData));
        otp_data->window = this;
        otp_data->slot = i;

        currPasswdItem =
            gtk_menu_item_new_with_label((const char *)cryptostick->TOTPSlots[i]->slotName);
        g_signal_connect(currPasswdItem, "activate", G_CALLBACK(onGetTOTP), otp_data);
        gtk_menu_shell_append(GTK_MENU_SHELL(passwordsSubMenu), currPasswdItem);
        gtk_widget_show(currPasswdItem);
      }
    }
    for (int i = 0; i < HOTP_SlotCount; i++) {
      GtkWidget *currPasswdItem;

      struct getOTPData *otp_data;

      if (cryptostick->HOTPSlots[i]->isProgrammed) {
        otp_data = (struct getOTPData *)malloc(sizeof(struct getOTPData));
        otp_data->window = this;
        otp_data->slot = i;

        currPasswdItem =
            gtk_menu_item_new_with_label((const char *)cryptostick->HOTPSlots[i]->slotName);
        g_signal_connect(currPasswdItem, "activate", G_CALLBACK(onGetHOTP), otp_data);
        gtk_menu_shell_append(GTK_MENU_SHELL(passwordsSubMenu), currPasswdItem);
        gtk_widget_show(currPasswdItem);
      }
    }
    if (TRUE == cryptostick->passwordSafeUnlocked) {
      for (int i = 0; i < PWS_SLOT_COUNT; i++) {
        GtkWidget *currPasswdItem;

        struct getOTPData *otp_data;

        if (cryptostick->passwordSafeStatus[i] == (unsigned char)true) {
          otp_data = (struct getOTPData *)malloc(sizeof(struct getOTPData));
          otp_data->window = this;
          otp_data->slot = i;

          currPasswdItem = gtk_menu_item_new_with_label(PWS_GetSlotName(i));
          g_signal_connect(currPasswdItem, "activate", G_CALLBACK(onGetPasswordSafeSlot), otp_data);
          gtk_menu_shell_append(GTK_MENU_SHELL(passwordsSubMenu), currPasswdItem);
          gtk_widget_show(currPasswdItem);
        }
      }
    }

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(passwordsItem), passwordsSubMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), separItem1);

    gtk_widget_show(passwordsItem);
    gtk_widget_show(passwordsSubMenu);
    gtk_widget_show(separItem1);
  } else
#endif // HAVE_LIBAPPINDICATOR
  {
    if (trayMenuPasswdSubMenu != NULL) {
      delete trayMenuPasswdSubMenu;
    }
    trayMenuPasswdSubMenu = new QMenu(tr("Passwords"));

    /* TOTP passwords */
    if (cryptostick->TOTPSlots[0]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[0]->slotName, this,
                                       SLOT(getTOTP1()));
    if (cryptostick->TOTPSlots[1]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[1]->slotName, this,
                                       SLOT(getTOTP2()));
    if (cryptostick->TOTPSlots[2]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[2]->slotName, this,
                                       SLOT(getTOTP3()));
    if (cryptostick->TOTPSlots[3]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[3]->slotName, this,
                                       SLOT(getTOTP4()));
    if (cryptostick->TOTPSlots[4]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[4]->slotName, this,
                                       SLOT(getTOTP5()));
    if (cryptostick->TOTPSlots[5]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[5]->slotName, this,
                                       SLOT(getTOTP6()));
    if (cryptostick->TOTPSlots[6]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[6]->slotName, this,
                                       SLOT(getTOTP7()));
    if (cryptostick->TOTPSlots[7]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[7]->slotName, this,
                                       SLOT(getTOTP8()));
    if (cryptostick->TOTPSlots[8]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[8]->slotName, this,
                                       SLOT(getTOTP9()));
    if (cryptostick->TOTPSlots[9]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[9]->slotName, this,
                                       SLOT(getTOTP10()));
    if (cryptostick->TOTPSlots[10]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[10]->slotName, this,
                                       SLOT(getTOTP11()));
    if (cryptostick->TOTPSlots[11]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[11]->slotName, this,
                                       SLOT(getTOTP12()));
    if (cryptostick->TOTPSlots[12]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[12]->slotName, this,
                                       SLOT(getTOTP13()));
    if (cryptostick->TOTPSlots[13]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[13]->slotName, this,
                                       SLOT(getTOTP14()));
    if (cryptostick->TOTPSlots[14]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->TOTPSlots[14]->slotName, this,
                                       SLOT(getTOTP15()));

    /* HOTP passwords */
    if (cryptostick->HOTPSlots[0]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->HOTPSlots[0]->slotName, this,
                                       SLOT(getHOTP1()));
    if (cryptostick->HOTPSlots[1]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->HOTPSlots[1]->slotName, this,
                                       SLOT(getHOTP2()));
    if (cryptostick->HOTPSlots[2]->isProgrammed == true)
      trayMenuPasswdSubMenu->addAction((char *)cryptostick->HOTPSlots[2]->slotName, this,
                                       SLOT(getHOTP3()));

    if (TRUE == cryptostick->passwordSafeUnlocked) {
      if (cryptostick->passwordSafeStatus[0] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(0), this, SLOT(PWS_Clicked_Slot00()));
      if (cryptostick->passwordSafeStatus[1] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(1), this, SLOT(PWS_Clicked_Slot01()));
      if (cryptostick->passwordSafeStatus[2] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(2), this, SLOT(PWS_Clicked_Slot02()));
      if (cryptostick->passwordSafeStatus[3] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(3), this, SLOT(PWS_Clicked_Slot03()));
      if (cryptostick->passwordSafeStatus[4] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(4), this, SLOT(PWS_Clicked_Slot04()));
      if (cryptostick->passwordSafeStatus[5] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(5), this, SLOT(PWS_Clicked_Slot05()));
      if (cryptostick->passwordSafeStatus[6] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(6), this, SLOT(PWS_Clicked_Slot06()));
      if (cryptostick->passwordSafeStatus[7] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(7), this, SLOT(PWS_Clicked_Slot07()));
      if (cryptostick->passwordSafeStatus[8] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(8), this, SLOT(PWS_Clicked_Slot08()));
      if (cryptostick->passwordSafeStatus[9] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(9), this, SLOT(PWS_Clicked_Slot09()));
      if (cryptostick->passwordSafeStatus[10] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(10), this, SLOT(PWS_Clicked_Slot10()));
      if (cryptostick->passwordSafeStatus[11] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(11), this, SLOT(PWS_Clicked_Slot11()));
      if (cryptostick->passwordSafeStatus[12] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(12), this, SLOT(PWS_Clicked_Slot12()));
      if (cryptostick->passwordSafeStatus[13] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(13), this, SLOT(PWS_Clicked_Slot13()));
      if (cryptostick->passwordSafeStatus[14] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(14), this, SLOT(PWS_Clicked_Slot14()));
      if (cryptostick->passwordSafeStatus[15] == (unsigned char)true)
        trayMenuPasswdSubMenu->addAction(PWS_GetSlotName(15), this, SLOT(PWS_Clicked_Slot15()));
    }

    if (!trayMenuPasswdSubMenu->actions().empty()) {
      trayMenu->addMenu(trayMenuPasswdSubMenu);
      trayMenu->addSeparator();
    }
  }
}

void MainWindow::generateMenuForProDevice() {
#ifdef HAVE_LIBAPPINDICATOR
  if (isUnity()) {
    GtkWidget *configureItem = gtk_image_menu_item_new_with_label(_("Configure"));
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(configureItem), TRUE);
    GtkWidget *configureItemImg = gtk_image_new_from_file("/usr/share/nitrokey/settings.png");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(configureItem), GTK_WIDGET(configureItemImg));

    GtkWidget *configurePasswordsItem;

    GtkWidget *changeUserPinItem = gtk_menu_item_new_with_label(_("Change user PIN"));
    GtkWidget *changeAdminPinItem = gtk_menu_item_new_with_label(_("Change admin PIN"));
    GtkWidget *resetUserPinItem = gtk_menu_item_new_with_label(_("Reset User PIN"));

    GtkWidget *resetItem = gtk_menu_item_new_with_label(_("Factory reset"));

    GtkWidget *separItem2 = gtk_separator_menu_item_new();

    GtkWidget *configureSubMenu = gtk_menu_new();

    g_signal_connect(changeUserPinItem, "activate", G_CALLBACK(onChangeUserPin), this);
    g_signal_connect(changeAdminPinItem, "activate", G_CALLBACK(onChangeAdminPin), this);
    g_signal_connect(resetUserPinItem, "activate", G_CALLBACK(onResetUserPin), this);
    g_signal_connect(resetItem, "activate", G_CALLBACK(onReset), this);

    generatePasswordMenu();
    generateMenuPasswordSafe();

    if (TRUE == cryptostick->passwordSafeAvailable)
      configurePasswordsItem = gtk_menu_item_new_with_label(_("OTP and Password safe"));
    else
      configurePasswordsItem = gtk_menu_item_new_with_label(_("OTP"));

    g_signal_connect(configurePasswordsItem, "activate", G_CALLBACK(onConfigure), this);

    gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), configureItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), configurePasswordsItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), changeUserPinItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), changeAdminPinItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), resetUserPinItem);
    if (ExtendedConfigActive) {
      gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), separItem2);
      gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), resetItem);
    }
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(configureItem), configureSubMenu);

    gtk_widget_show(separItem2);
    gtk_widget_show(configureItem);
    gtk_widget_show(configurePasswordsItem);
    gtk_widget_show(changeUserPinItem);
    gtk_widget_show(changeAdminPinItem);

    cryptostick->getUserPasswordRetryCount();
    // if (0 == cryptostick->getUserPasswordRetryCount() )
    if (0 == HID_Stick20Configuration_st.UserPwRetryCount) // cryptostick->userPasswordRetryCount)
      gtk_widget_show(resetUserPinItem);

    gtk_widget_show(resetItem);
    gtk_widget_show(configureSubMenu);
  } else
#endif // HAVE_LIBAPPINDICATOR
  {
    generatePasswordMenu();
    trayMenu->addSeparator();
    generateMenuPasswordSafe();

    trayMenuSubConfigure = trayMenu->addMenu(tr("Configure"));
    trayMenuSubConfigure->setIcon(QIcon(":/images/settings.png"));

    if (TRUE == cryptostick->passwordSafeAvailable)
      trayMenuSubConfigure->addAction(configureActionStick20);
    else
      trayMenuSubConfigure->addAction(configureAction);

    trayMenuSubConfigure->addSeparator();

    trayMenuSubConfigure->addAction(Stick10ActionChangeUserPIN);
    trayMenuSubConfigure->addAction(Stick10ActionChangeAdminPIN);

    // Enable "reset user PIN" ?
    cryptostick->getUserPasswordRetryCount();
    if (0 == HID_Stick20Configuration_st.UserPwRetryCount) // cryptostick->userPasswordRetryCount)
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

#ifdef HAVE_LIBAPPINDICATOR
  if (isUnity()) {
    GtkWidget *updateStorageStatusItem =
        gtk_menu_item_new_with_label(_("Smartcard or SD card are not ready"));
    GtkWidget *initEncryptedVolumeItem = gtk_menu_item_new_with_label(_("Initialize device"));
    GtkWidget *fillSDCardWithRandomCharsItem =
        gtk_menu_item_new_with_label(_("Initialize storage with random data"));
    GtkWidget *enableEncryptedVolumeItem =
        gtk_menu_item_new_with_label(_("Unlock encrypted volume"));
    GtkWidget *disableEncryptedVolumeItem =
        gtk_menu_item_new_with_label(_("Lock encrypted volume"));
    GtkWidget *enableHiddenVolumeItem = gtk_menu_item_new_with_label(_("Unlock hidden volume"));
    GtkWidget *disableHiddenVolumeItem = gtk_menu_item_new_with_label(_("Lock hidden volume"));
    GtkWidget *setupHiddenVolumeItem = gtk_menu_item_new_with_label(_("Setup hidden volume"));
    GtkWidget *lockDeviceItem = gtk_menu_item_new_with_label(_("Lock device"));

    GtkWidget *setReadOnlyUnencryptedVolumeItem =
        gtk_menu_item_new_with_label(_("Set unencrypted volume read-only"));
    GtkWidget *setReadWriteUnencryptedVolumeItem =
        gtk_menu_item_new_with_label(_("Set unencrypted volume read-write"));
    GtkWidget *destroyEncryptedVolumeItem =
        gtk_menu_item_new_with_label(_("Destroy encrypted data"));
    GtkWidget *enableFirmwareUpdateItem = gtk_menu_item_new_with_label(_("Enable firmware update"));
    GtkWidget *exportFirmwareToFileItem =
        gtk_menu_item_new_with_label(_("Export firmware to file"));
    GtkWidget *lockHardwareItem = gtk_menu_item_new_with_label(_("Lock hardware"));

    GtkWidget *resetUserPasswordItem = gtk_menu_item_new_with_label(_("Reset User PIN"));
    GtkWidget *debugItem = gtk_menu_item_new_with_label(_("Debug"));

    GtkWidget *clearNewSDCardFoundItem =
        gtk_menu_item_new_with_label(_("Disable 'Initialize storage with random data' warning"));
    GtkWidget *configureItem = gtk_image_menu_item_new_with_label(_("Configure"));
    gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(configureItem), TRUE);
    GtkWidget *configureItemImg = gtk_image_new_from_file("/usr/share/nitrokey/settings.png");
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(configureItem), GTK_WIDGET(configureItemImg));

    GtkWidget *extendedConfigureItem = gtk_menu_item_new_with_label(_("Advanced configure"));

    GtkWidget *setupPasswordMatrixItem = gtk_menu_item_new_with_label("");

    GtkWidget *configurePasswordsItem;

    GtkWidget *changeUserPinItem = gtk_menu_item_new_with_label(_("Change User PIN"));
    GtkWidget *changeAdminPinItem = gtk_menu_item_new_with_label(_("Change Admin PIN"));
    GtkWidget *changeUpdatePinItem = gtk_menu_item_new_with_label(_("Change Firmware Password"));

    GtkWidget *separItem1 = gtk_separator_menu_item_new();

    GtkWidget *separItem2 = gtk_separator_menu_item_new();

    GtkWidget *configureSubMenu = gtk_menu_new();

    GtkWidget *extendedConfigureSubMenu = gtk_menu_new();

    g_signal_connect(updateStorageStatusItem, "activate", G_CALLBACK(onAbout), this);
    g_signal_connect(initEncryptedVolumeItem, "activate", G_CALLBACK(onInitEncryptedVolume), this);
    g_signal_connect(fillSDCardWithRandomCharsItem, "activate",
                     G_CALLBACK(onFillSDCardWithRandomChars), this);
    g_signal_connect(enableEncryptedVolumeItem, "activate", G_CALLBACK(onEnableEncryptedVolume),
                     this);
    g_signal_connect(disableEncryptedVolumeItem, "activate", G_CALLBACK(onDisableEncryptedVolume),
                     this);
    g_signal_connect(enableHiddenVolumeItem, "activate", G_CALLBACK(onEnableHiddenVolume), this);
    g_signal_connect(disableHiddenVolumeItem, "activate", G_CALLBACK(onDisableHiddenVolume), this);
    g_signal_connect(setupHiddenVolumeItem, "activate", G_CALLBACK(onSetupHiddenVolumeItem), this);
    g_signal_connect(lockDeviceItem, "activate", G_CALLBACK(onLockDevice), this);
    g_signal_connect(setReadOnlyUnencryptedVolumeItem, "activate",
                     G_CALLBACK(onSetReadOnlyUnencryptedVolumeItem), this);
    g_signal_connect(setReadWriteUnencryptedVolumeItem, "activate",
                     G_CALLBACK(onSetReadWriteUnencryptedVolumeItem), this);
    g_signal_connect(destroyEncryptedVolumeItem, "activate", G_CALLBACK(onInitEncryptedVolume),
                     this);
    g_signal_connect(enableFirmwareUpdateItem, "activate", G_CALLBACK(onEnableFirmwareUpdate),
                     this);
    g_signal_connect(exportFirmwareToFileItem, "activate", G_CALLBACK(onExportFirmwareToFile),
                     this);
    g_signal_connect(lockHardwareItem, "activate", G_CALLBACK(onLockHardware), this);
    g_signal_connect(debugItem, "activate", G_CALLBACK(onDebug), this);
    g_signal_connect(setupPasswordMatrixItem, "activate", G_CALLBACK(onSetupPasswordMatrix), this);
    g_signal_connect(clearNewSDCardFoundItem, "activate", G_CALLBACK(onClearNewSDCardFound), this);
    g_signal_connect(resetUserPasswordItem, "activate", G_CALLBACK(onResetUserPassword), this);
    g_signal_connect(changeUserPinItem, "activate", G_CALLBACK(onChangeUserPinStorage), this);
    g_signal_connect(changeAdminPinItem, "activate", G_CALLBACK(onChangeAdminPinStorage), this);
    g_signal_connect(changeUpdatePinItem, "activate", G_CALLBACK(onChangeUpdatePin), this);

    if (FALSE == Stick20ScSdCardOnline) // Is Stick 2.0 online (SD + SC
                                        // accessable?)
    {
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), updateStorageStatusItem);
      return;
    }

    if (TRUE == StickNotInitated) {
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), initEncryptedVolumeItem);
      AddSeperator = TRUE;
    }

    if (FALSE == StickNotInitated && TRUE == SdCardNotErased) {
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), fillSDCardWithRandomCharsItem);
      AddSeperator = TRUE;
    }

    if (TRUE == AddSeperator)
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), separItem1);

    generatePasswordMenu();
    if (FALSE == StickNotInitated) {
      // Enable tab for password safe for stick 2
      if (-1 == ui->tabWidget->indexOf(ui->tab_3))
        ui->tabWidget->addTab(ui->tab_3, tr("Password Safe"));

      // Setup entrys for password safe
      generateMenuPasswordSafe();
    }

    if (FALSE == SdCardNotErased) {
      if (FALSE == CryptedVolumeActive)
        gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), enableEncryptedVolumeItem);
      else
        gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), disableEncryptedVolumeItem);

      if (FALSE == HiddenVolumeActive)
        gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), enableHiddenVolumeItem);
      else
        gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), disableHiddenVolumeItem);
    }

    if (FALSE != (HiddenVolumeActive || CryptedVolumeActive || PasswordSafeEnabled))
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), lockDeviceItem);

    if (TRUE == cryptostick->passwordSafeAvailable)
      configurePasswordsItem = gtk_menu_item_new_with_label("OTP and Password safe");
    else
      configurePasswordsItem = gtk_menu_item_new_with_label("OTP");

    g_signal_connect(configurePasswordsItem, "activate", G_CALLBACK(onConfigure), this);

    gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), configureItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), configurePasswordsItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), changeUserPinItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), changeAdminPinItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), changeUpdatePinItem);

    if (TRUE == MatrixInputActive)
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), setupPasswordMatrixItem);

    // Storage actions
    if (FALSE == NormalVolumeRWActive)
      gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), setReadOnlyUnencryptedVolumeItem);
    else
      gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), setReadWriteUnencryptedVolumeItem);

    if (FALSE == SdCardNotErased)
      gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), setupHiddenVolumeItem);

    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), destroyEncryptedVolumeItem);

    // Other actions
    if (TRUE == LockHardware)
      gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), lockHardwareItem);

    if (TRUE == HiddenVolumeAccessable) {
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), enableFirmwareUpdateItem);
    gtk_menu_shell_append(GTK_MENU_SHELL(configureSubMenu), exportFirmwareToFileItem);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(configureItem), configureSubMenu);

    if (TRUE == ExtendedConfigActive) {
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), extendedConfigureItem);
      gtk_menu_shell_append(GTK_MENU_SHELL(extendedConfigureSubMenu),
                            fillSDCardWithRandomCharsItem);
      if (TRUE == SdCardNotErased)
        gtk_menu_shell_append(GTK_MENU_SHELL(extendedConfigureSubMenu), clearNewSDCardFoundItem);

      gtk_menu_item_set_submenu(GTK_MENU_ITEM(extendedConfigureItem), extendedConfigureSubMenu);
      gtk_widget_show(extendedConfigureItem);
      gtk_widget_show(extendedConfigureSubMenu);
      gtk_widget_show(fillSDCardWithRandomCharsItem);
      gtk_widget_show(clearNewSDCardFoundItem);
    }

    // Enable "reset user PIN" ?
    if (0 == cryptostick->userPasswordRetryCount) {
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), resetUserPasswordItem);
    }

    // Add debug window ?
    if (TRUE == DebugWindowActive) {
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), debugItem);
    }

    // Setup OTP combo box
    generateComboBoxEntrys();

    gtk_widget_show(enableFirmwareUpdateItem);
    gtk_widget_show(exportFirmwareToFileItem);
    gtk_widget_show(updateStorageStatusItem);
    gtk_widget_show(initEncryptedVolumeItem);
    gtk_widget_show(fillSDCardWithRandomCharsItem);
    gtk_widget_show(enableEncryptedVolumeItem);
    gtk_widget_show(disableEncryptedVolumeItem);
    gtk_widget_show(enableHiddenVolumeItem);
    gtk_widget_show(disableHiddenVolumeItem);
    gtk_widget_show(setupHiddenVolumeItem);
    gtk_widget_show(lockDeviceItem);
    gtk_widget_show(setReadOnlyUnencryptedVolumeItem);
    gtk_widget_show(setReadWriteUnencryptedVolumeItem);
    gtk_widget_show(destroyEncryptedVolumeItem);
    gtk_widget_show(lockHardwareItem);
    gtk_widget_show(resetUserPasswordItem);
    gtk_widget_show(debugItem);
    gtk_widget_show(configureItem);
    gtk_widget_show(configurePasswordsItem);
    gtk_widget_show(changeUserPinItem);
    gtk_widget_show(changeAdminPinItem);
    gtk_widget_show(changeUpdatePinItem);
    gtk_widget_show(configureSubMenu);
    gtk_widget_show(separItem1);
    gtk_widget_show(separItem2);
  } else
#endif // HAVE_LIBAPPINDICATOR
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
    if (0 == cryptostick->userPasswordRetryCount) {
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

void MainWindow::generateHOTPConfig(HOTPSlot *slot) {
  uint8_t selectedSlot = ui->slotComboBox->currentIndex();
  selectedSlot -= (TOTP_SlotCount + 1);

  if (selectedSlot < HOTP_SlotCount) {
    slot->slotNumber = selectedSlot + 0x10;

    QByteArray secretFromGUI = ui->secretEdit->text().toLatin1();

    uint8_t encoded[128];
    uint8_t decoded[20];
    uint8_t data[128];

    memset(encoded, 'A', 32);
    memset(data, 'A', 32);
    memcpy(encoded, secretFromGUI.data(), secretFromGUI.length());

    base32_clean(encoded, 32, data);
    base32_decode(data, decoded, 20);

    secretFromGUI = QByteArray((char *)decoded, 20); // .toHex();
    memset(slot->secret, 0, 20);
    memcpy(slot->secret, secretFromGUI.data(), secretFromGUI.size());

    QByteArray slotNameFromGUI = QByteArray(ui->nameEdit->text().toLatin1());
    memset(slot->slotName, 0, 15);
    memcpy(slot->slotName, slotNameFromGUI.data(), slotNameFromGUI.size());

    memset(slot->tokenID, 32, 13);
    QByteArray ompFromGUI = (ui->ompEdit->text().toLatin1());
    memcpy(slot->tokenID, ompFromGUI, 2);

    QByteArray ttFromGUI = (ui->ttEdit->text().toLatin1());
    memcpy(slot->tokenID + 2, ttFromGUI, 2);

    QByteArray muiFromGUI = (ui->muiEdit->text().toLatin1());
    memcpy(slot->tokenID + 4, muiFromGUI, 8);

    slot->tokenID[12] = ui->keyboardComboBox->currentIndex() & 0xFF;

    memset(slot->counter, 0, 8);
    // Nitrokey Storage needs counter value in text but Pro in binary [#60]
    if (cryptostick->activStick20 == false) {
      bool conversionSuccess = false;
      uint64_t counterFromGUI = 0;
      if (0 != ui->counterEdit->text().toLatin1().length()) {
        counterFromGUI = ui->counterEdit->text().toLatin1().toULongLong(&conversionSuccess);
      }
      if (conversionSuccess) {
        // FIXME check for little endian/big endian conversion (test on Macintosh)
        memcpy(slot->counter, &counterFromGUI, sizeof counterFromGUI);
      } else {
        csApplet->warningBox(tr("Counter value not copied - there was an error in conversion. "
                                "Setting counter value to 0. Please retry."));
      }
    } else { // nitrokey storage version
      QByteArray counterFromGUI = QByteArray(ui->counterEdit->text().toLatin1());
      int digitsInCounter = counterFromGUI.length();
      if (0 < digitsInCounter && digitsInCounter < 8) {
        memcpy(slot->counter, counterFromGUI.data(), std::min(counterFromGUI.length(), 7));
        // 8th char has to be '\0' since in firmware atoi is used directly on buffer
        slot->counter[7] = 0;
      } else {
        csApplet->warningBox(
            tr("Counter value not copied - Nitrokey Storage handles HOTP counter "
               "values up to 7 digits. Setting counter value to 0. Please retry."));
      }
    }
    if (DebugingActive) {
      if (cryptostick->activStick20)
        qDebug() << "HOTP counter value: " << *(char *)slot->counter;
      else
        qDebug() << "HOTP counter value: " << *(quint64 *)slot->counter;
    }
    slot->config = 0;

    if (TRUE == ui->digits8radioButton->isChecked())
      slot->config += (1 << 0);

    if (TRUE == ui->enterCheckBox->isChecked())
      slot->config += (1 << 1);

    if (TRUE == ui->tokenIDCheckBox->isChecked())
      slot->config += (1 << 2);
  }
}

void MainWindow::generateTOTPConfig(TOTPSlot *slot) {
  uint8_t selectedSlot = ui->slotComboBox->currentIndex();

  // get the TOTP slot number
  if (selectedSlot < TOTP_SlotCount) {
    slot->slotNumber = selectedSlot + 0x20;

    QByteArray secretFromGUI = ui->secretEdit->text().toLatin1();

    uint8_t encoded[128];

    uint8_t decoded[20];

    uint8_t data[128];

    memset(encoded, 'A', 32);
    memset(data, 'A', 32);
    memcpy(encoded, secretFromGUI.data(), secretFromGUI.length());

    base32_clean(encoded, 32, data);
    base32_decode(data, decoded, 20);

    secretFromGUI = QByteArray((char *)decoded, 20); // .toHex();

    memset(slot->secret, 0, 20);
    memcpy(slot->secret, secretFromGUI.data(), secretFromGUI.size());

    QByteArray slotNameFromGUI = QByteArray(ui->nameEdit->text().toLatin1());

    memset(slot->slotName, 0, 15);
    memcpy(slot->slotName, slotNameFromGUI.data(), slotNameFromGUI.size());

    memset(slot->tokenID, 32, 13);

    QByteArray ompFromGUI = (ui->ompEdit->text().toLatin1());

    memcpy(slot->tokenID, ompFromGUI, 2);

    QByteArray ttFromGUI = (ui->ttEdit->text().toLatin1());

    memcpy(slot->tokenID + 2, ttFromGUI, 2);

    QByteArray muiFromGUI = (ui->muiEdit->text().toLatin1());

    memcpy(slot->tokenID + 4, muiFromGUI, 8);

    slot->config = 0;

    if (ui->digits8radioButton->isChecked())
      slot->config += (1 << 0);
    if (ui->enterCheckBox->isChecked())
      slot->config += (1 << 1);
    if (ui->tokenIDCheckBox->isChecked())
      slot->config += (1 << 2);

    uint16_t lastInterval = ui->intervalSpinBox->value();

    if (lastInterval < 1)
      lastInterval = 1;

    slot->interval = lastInterval;
  }
}

void MainWindow::generateAllConfigs() {
  cryptostick->initializeConfig();
  cryptostick->getSlotConfigs();
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

  ui->nameEdit->setText(QString((char *)cryptostick->TOTPSlots[slotNo]->slotName));
  QByteArray secret((char *)cryptostick->TOTPSlots[slotNo]->secret, 20);

  ui->base32RadioButton->setChecked(true);
  ui->secretEdit->setText(secret); // .toHex());

  ui->counterEdit->setText("0");

  QByteArray omp((char *)cryptostick->TOTPSlots[slotNo]->tokenID, 2);

  ui->ompEdit->setText(QString(omp));

  QByteArray tt((char *)cryptostick->TOTPSlots[slotNo]->tokenID + 2, 2);

  ui->ttEdit->setText(QString(tt));

  QByteArray mui((char *)cryptostick->TOTPSlots[slotNo]->tokenID + 4, 8);

  ui->muiEdit->setText(QString(mui));

  int interval = cryptostick->TOTPSlots[slotNo]->interval;

  ui->intervalSpinBox->setValue(interval);

  if (cryptostick->TOTPSlots[slotNo]->config & (1 << 0))
    ui->digits8radioButton->setChecked(true);
  else
    ui->digits6radioButton->setChecked(true);

  if (cryptostick->TOTPSlots[slotNo]->config & (1 << 1))
    ui->enterCheckBox->setChecked(true);
  else
    ui->enterCheckBox->setChecked(false);

  if (cryptostick->TOTPSlots[slotNo]->config & (1 << 2))
    ui->tokenIDCheckBox->setChecked(true);
  else
    ui->tokenIDCheckBox->setChecked(false);

  if (!cryptostick->TOTPSlots[slotNo]->isProgrammed) {
    ui->ompEdit->setText("NK");
    ui->ttEdit->setText("01");
    QByteArray cardSerial = QByteArray((char *)cryptostick->cardSerial).toHex();
    ui->muiEdit->setText(QString("%1").arg(QString(cardSerial), 8, '0'));
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
  ui->nameEdit->setText(QString((char *)cryptostick->HOTPSlots[slotNo]->slotName));
  QByteArray secret((char *)cryptostick->HOTPSlots[slotNo]->secret, 20);

  ui->base32RadioButton->setChecked(true);
  ui->secretEdit->setText(secret); // .toHex());

  if (cryptostick->activStick20) {
    QByteArray counter((char *)cryptostick->HOTPSlots[slotNo]->counter, 8);
    QString TextCount;
    TextCount = QString("%1").arg(counter.toInt());
    ui->counterEdit->setText(TextCount); // .toHex());
  } else {
    QString TextCount = QString("%1").arg(*(qulonglong *)cryptostick->HOTPSlots[slotNo]->counter);
    ui->counterEdit->setText(TextCount);
  }
  QByteArray omp((char *)cryptostick->HOTPSlots[slotNo]->tokenID, 2);

  ui->ompEdit->setText(QString(omp));

  QByteArray tt((char *)cryptostick->HOTPSlots[slotNo]->tokenID + 2, 2);

  ui->ttEdit->setText(QString(tt));

  QByteArray mui((char *)cryptostick->HOTPSlots[slotNo]->tokenID + 4, 8);

  ui->muiEdit->setText(QString(mui));

  if (cryptostick->HOTPSlots[slotNo]->config & (1 << 0))
    ui->digits8radioButton->setChecked(true);
  else
    ui->digits6radioButton->setChecked(true);

  if (cryptostick->HOTPSlots[slotNo]->config & (1 << 1))
    ui->enterCheckBox->setChecked(true);
  else
    ui->enterCheckBox->setChecked(false);

  if (cryptostick->HOTPSlots[slotNo]->config & (1 << 2))
    ui->tokenIDCheckBox->setChecked(true);
  else
    ui->tokenIDCheckBox->setChecked(false);

  if (!cryptostick->HOTPSlots[slotNo]->isProgrammed) {
    ui->ompEdit->setText("NK");
    ui->ttEdit->setText("01");
    QByteArray cardSerial = QByteArray((char *)cryptostick->cardSerial).toHex();
    ui->muiEdit->setText(QString("%1").arg(QString(cardSerial), 8, '0'));
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

  if (cryptostick->generalConfig[0] == 0 || cryptostick->generalConfig[0] == 1)
    ui->numLockComboBox->setCurrentIndex(cryptostick->generalConfig[0] + 1);

  if (cryptostick->generalConfig[1] == 0 || cryptostick->generalConfig[1] == 1)
    ui->capsLockComboBox->setCurrentIndex(cryptostick->generalConfig[1] + 1);

  if (cryptostick->generalConfig[2] == 0 || cryptostick->generalConfig[2] == 1)
    ui->scrollLockComboBox->setCurrentIndex(cryptostick->generalConfig[2] + 1);

  if (cryptostick->otpPasswordConfig[0] == 1)
    ui->enableUserPasswordCheckBox->setChecked(true);
  else
    ui->enableUserPasswordCheckBox->setChecked(false);

  if (cryptostick->otpPasswordConfig[1] == 1)
    ui->deleteUserPasswordCheckBox->setChecked(true);
  else
    ui->deleteUserPasswordCheckBox->setChecked(false);

  lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
}

void MainWindow::startConfiguration() {
  bool ok;

  if (!cryptostick->validPassword) {
    do {
      PinDialog dialog(tr("Enter card admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
                       PinDialog::ADMIN_PIN);
      ok = dialog.exec();
      QString password;

      dialog.getPassword(password);

      if (QDialog::Accepted == ok) {
        uint8_t tempPassword[25];

        for (int i = 0; i < 25; i++)
          tempPassword[i] = qrand() & 0xFF;

        cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(), tempPassword);
        if (cryptostick->validPassword) {
          lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
        } else {
          csApplet->warningBox(tr("Wrong PIN. Please try again."));
        }
        password.clear();
      }
    } while (QDialog::Accepted == ok && !cryptostick->validPassword);
  }

  // Start the config dialog
  if (cryptostick->validPassword) {
    cryptostick->getSlotConfigs();
    displayCurrentSlotConfig();

    cryptostick->getStatus();
    displayCurrentGeneralConfig();

    SetupPasswordSafeConfig();

    raise();
    activateWindow();
    showNormal();
    setWindowState(Qt::WindowActive);

    QTimer::singleShot(0, this, SLOT(resizeMin()));
  }
  if (cryptostick->activStick20) {
    ui->counterEdit->setMaxLength(7);
  }
}

void MainWindow::resizeMin() { resize(minimumSizeHint()); }

void MainWindow::destroyPasswordSafeStick10() {
  uint8_t password[40];
  bool ret;
  int ret_s32;
  QMessageBox msgBox;
  PasswordDialog dialog(FALSE, this);

  cryptostick->getUserPasswordRetryCount();
  dialog.init((char *)(tr("Enter admin PIN").toUtf8().data()),
              HID_Stick20Configuration_st.UserPwRetryCount);
  dialog.cryptostick = cryptostick;
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    dialog.getPassword((char *)password);

    ret_s32 = cryptostick->buildAesKey((uint8_t *)&(password[1]));

    if (CMD_STATUS_OK == ret_s32) {
      msgBox.setText(tr("AES key generated"));
      msgBox.exec();
    } else {
      if (CMD_STATUS_WRONG_PASSWORD == ret_s32)
        msgBox.setText(tr("Wrong password"));
      else
        msgBox.setText(tr("Unable to create AES key"));

      msgBox.exec();
    }
  }
}

void MainWindow::startStick20Configuration() {
  Stick20Dialog dialog(this);

  dialog.cryptostick = cryptostick;
  dialog.exec();
}

void MainWindow::startStickDebug() {
  DebugDialog dialog(this);

  dialog.cryptostick = cryptostick;
  dialog.updateText(); // Init data
  dialog.exec();
}

void MainWindow::refreshStick20StatusData() {
  if (TRUE == cryptostick->activStick20) {
    // Get actual data from stick 20
    cryptostick->stick20GetStatusData();
    Stick20ResponseTask ResponseTask(this, cryptostick, trayIcon);
    ResponseTask.NoStopWhenStatusOK();
    ResponseTask.GetResponse();
    UpdateDynamicMenuEntrys(); // Use new data to update menu
  }
}

void MainWindow::startAboutDialog() {
  refreshStick20StatusData();
  AboutDialog dialog(cryptostick, this);
  dialog.exec();
}

void MainWindow::startStick20Setup() {
  Stick20Setup dialog(this);

  dialog.cryptostick = cryptostick;
  dialog.exec();
}

void MainWindow::startMatrixPasswordDialog() {
  MatrixPasswordDialog dialog(this);

  dialog.cryptostick = cryptostick;
  dialog.PasswordLen = 6;
  dialog.SetupInterfaceFlag = FALSE;
  dialog.InitSecurePasswordDialog();
  dialog.exec();
}

void MainWindow::startStick20EnableCryptedVolume() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool ret;

  bool answer;

  if (TRUE == HiddenVolumeActive) {
    answer = csApplet->yesOrNoBox(
        tr("This activity locks your hidden volume. Do you want to "
           "proceed?\nTo avoid data loss, please unmount the partitions before "
           "proceeding."),
        0, false);
    if (false == answer)
      return;
  }

  PinDialog dialog(tr("User pin dialog"), tr("Enter user PIN:"), cryptostick, PinDialog::PREFIXED,
                   PinDialog::USER_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    local_sync();
    dialog.getPassword((char *)password);
    stick20SendCommand(STICK20_CMD_ENABLE_CRYPTED_PARI, password);
  }
}

void MainWindow::startStick20DisableCryptedVolume() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool answer;

  if (TRUE == CryptedVolumeActive) {
    answer = csApplet->yesOrNoBox(
        tr("This activity locks your encrypted volume. Do you want to "
           "proceed?\nTo avoid data loss, please unmount the partitions before "
           "proceeding."),
        0, false);
    if (false == answer)
      return;

    local_sync();
    password[0] = 0;
    stick20SendCommand(STICK20_CMD_DISABLE_CRYPTED_PARI, password);
  }
}

void MainWindow::startStick20EnableHiddenVolume() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool ret;

  bool answer;

  if (FALSE == CryptedVolumeActive) {
    csApplet->warningBox(tr("Please enable the encrypted volume first."));
    return;
  }

  answer =
      csApplet->yesOrNoBox(tr("This activity locks your encrypted volume. Do you want to "
                              "proceed?\nTo avoid data loss, please unmount the partitions before "
                              "proceeding."),
                           0, true);
  if (false == answer)
    return;

  PinDialog dialog(tr("Enter password for hidden volume"), tr("Enter password for hidden volume:"),
                   cryptostick, PinDialog::PREFIXED, PinDialog::OTHER);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    local_sync();
    // password[0] = 'P';
    dialog.getPassword((char *)password);

    stick20SendCommand(STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI, password);
  }
}

void MainWindow::startStick20DisableHiddenVolume() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool answer;

  answer =
      csApplet->yesOrNoBox(tr("This activity locks your hidden volume. Do you want to proceed?\nTo "
                              "avoid data loss, please unmount the partitions before proceeding."),
                           0, true);
  if (false == answer)
    return;

  local_sync();
  password[0] = 0;
  stick20SendCommand(STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI, password);
}

void MainWindow::startLockDeviceAction() {
  bool answer;

  if ((TRUE == CryptedVolumeActive) || (TRUE == HiddenVolumeActive)) {
    answer = csApplet->yesOrNoBox(
        tr("This activity locks your encrypted volume. Do you want to "
           "proceed?\nTo avoid data loss, please unmount the partitions before "
           "proceeding."),
        0, true);
    if (false == answer) {
      return;
    }
    local_sync();
  }

  if (cryptostick->lockDevice()) {
    cryptostick->passwordSafeUnlocked = false;
  }

  HID_Stick20Configuration_st.VolumeActiceFlag_u8 = 0;
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

  PinDialog dialog(tr("Enter Firmware Password"), tr("Enter Firmware Password:"), cryptostick,
                   PinDialog::PREFIXED, PinDialog::FIRMWARE_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    // FIXME unmount all volumes and sync
    dialog.getPassword((char *)password);

    stick20SendCommand(STICK20_CMD_ENABLE_FIRMWARE_UPDATE, password);
  }
}

void MainWindow::startStick10ActionChangeUserPIN() {
  DialogChangePassword dialog(this);

  dialog.setModal(TRUE);

  dialog.cryptostick = cryptostick;

  dialog.PasswordKind = STICK10_PASSWORD_KIND_USER;

  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick10ActionChangeAdminPIN() {
  DialogChangePassword dialog(this);

  dialog.setModal(TRUE);
  dialog.cryptostick = cryptostick;
  dialog.PasswordKind = STICK10_PASSWORD_KIND_ADMIN;
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ActionChangeUpdatePIN() {
  DialogChangePassword dialog(this);

  dialog.setModal(TRUE);
  dialog.cryptostick = cryptostick;
  dialog.PasswordKind = STICK20_PASSWORD_KIND_UPDATE;
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ActionChangeUserPIN() {
  DialogChangePassword dialog(this);

  dialog.setModal(TRUE);

  dialog.cryptostick = cryptostick;

  dialog.PasswordKind = STICK20_PASSWORD_KIND_USER;

  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ActionChangeAdminPIN() {
  DialogChangePassword dialog(this);

  dialog.setModal(TRUE);

  dialog.cryptostick = cryptostick;

  dialog.PasswordKind = STICK20_PASSWORD_KIND_ADMIN;

  dialog.InitData();
  dialog.exec();
}

void MainWindow::startResetUserPassword() {
  DialogChangePassword dialog(this);

  dialog.setModal(TRUE);

  dialog.cryptostick = cryptostick;

  if (cryptostick->activStick20)
    dialog.PasswordKind = STICK20_PASSWORD_KIND_RESET_USER;
  else
    dialog.PasswordKind = STICK10_PASSWORD_KIND_RESET_USER;

  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ExportFirmwareToFile() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool ret;

  PinDialog dialog(tr("Enter admin PIN"), tr("Enter admin PIN:"), cryptostick, PinDialog::PREFIXED,
                   PinDialog::ADMIN_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    // password[0] = 'P';
    dialog.getPassword((char *)password);

    stick20SendCommand(STICK20_CMD_EXPORT_FIRMWARE_TO_FILE, password);
  }
}

void MainWindow::startStick20DestroyCryptedVolume(int fillSDWithRandomChars) {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  int ret;

  bool answer;

  answer = csApplet->yesOrNoBox(
      tr("WARNING: Generating new AES keys will destroy the encrypted volumes, "
         "hidden volumes, and password safe! Continue?"),
      0, false);
  if (true == answer) {
    PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PREFIXED,
                     PinDialog::ADMIN_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret) {
      dialog.getPassword((char *)password);

      bool success = stick20SendCommand(STICK20_CMD_GENERATE_NEW_KEYS, password);
      if (success && fillSDWithRandomChars != 0) {
        stick20SendCommand(STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS, password);
      }
      refreshStick20StatusData();
    }
  }
}

void MainWindow::startStick20FillSDCardWithRandomChars() {
  uint8_t password[40];

  bool ret;

  PinDialog dialog(tr("Enter admin PIN"), tr("Admin Pin:"), cryptostick, PinDialog::PREFIXED,
                   PinDialog::ADMIN_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    // password[0] = 'P';
    dialog.getPassword((char *)password);

    stick20SendCommand(STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS, password);
  }
}

void MainWindow::startStick20ClearNewSdCardFound() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool ret;

  PinDialog dialog(tr("Enter admin PIN"), tr("Enter admin PIN:"), cryptostick, PinDialog::PREFIXED,
                   PinDialog::ADMIN_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    dialog.getPassword((char *)password);

    stick20SendCommand(STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND, password);
  }
}

void MainWindow::startStick20GetStickStatus() {

  // Get actual data from stick 20
  cryptostick->stick20GetStatusData();

  // Wait for response
  Stick20ResponseTask ResponseTask(this, cryptostick, trayIcon);

  ResponseTask.NoStopWhenStatusOK();
  ResponseTask.GetResponse();

  /*
     Stick20ResponseDialog ResponseDialog(this);

     ResponseDialog.NoStopWhenStatusOK ();

     ResponseDialog.cryptostick = cryptostick; ResponseDialog.exec();
     ResponseDialog.ResultValue; */
  Stick20InfoDialog InfoDialog(this);

  InfoDialog.exec();
}

void MainWindow::startStick20SetReadOnlyUncryptedVolume() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool ret;

  PinDialog dialog(tr("Enter user PIN"), tr("User PIN:"), cryptostick, PinDialog::PREFIXED,
                   PinDialog::USER_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    dialog.getPassword((char *)password);

    stick20SendCommand(STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN, password);
  }
}

void MainWindow::startStick20SetReadWriteUncryptedVolume() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool ret;

  PinDialog dialog(tr("Enter user PIN"), tr("User PIN:"), cryptostick, PinDialog::PREFIXED,
                   PinDialog::USER_PIN);
  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    dialog.getPassword((char *)password);

    stick20SendCommand(STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN, password);
  }
}

void MainWindow::startStick20LockStickHardware() {
  uint8_t password[LOCAL_PASSWORD_SIZE];

  bool ret;

  stick20LockFirmwareDialog dialog(this);

  ret = dialog.exec();
  if (QDialog::Accepted == ret) {
    PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PREFIXED,
                     PinDialog::ADMIN_PIN);
    ret = dialog.exec();

    if (QDialog::Accepted == ret) {
      dialog.getPassword((char *)password);
      stick20SendCommand(STICK20_CMD_SEND_LOCK_STICK_HARDWARE, password);
    }
  }
}

void MainWindow::startStick20SetupPasswordMatrix() {
  MatrixPasswordDialog dialog(this);

  csApplet->warningBox(tr("The selected lines must be greater then greatest password length"));

  dialog.setModal(TRUE);

  dialog.cryptostick = cryptostick;
  dialog.SetupInterfaceFlag = TRUE;

  dialog.InitSecurePasswordDialog();

  dialog.exec();
}

void MainWindow::startStick20DebugAction() {
  int ret;

  /*
     securitydialog dialog(this); ret = dialog.exec();
     csApplet->warningBox("Encrypted volume is not secure.\nSelect \"Initialize
     keys\"");

     StickNotInitated = TRUE; generateMenu(); */

  // cryptostick->stick20NewUpdatePassword((uint8_t *)"12345678",(uint8_t
  // *)"123456");
}

void MainWindow::startStick20SetupHiddenVolume() {
  bool ret;

  stick20HiddenVolumeDialog HVDialog(this);

  if (FALSE == CryptedVolumeActive) {
    csApplet->warningBox(tr("Please enable the encrypted volume first."));
    return;
  }

  HVDialog.SdCardHighWatermark_Read_Min = 0;
  HVDialog.SdCardHighWatermark_Read_Max = 100;
  HVDialog.SdCardHighWatermark_Write_Min = 0;
  HVDialog.SdCardHighWatermark_Write_Max = 100;

  ret = cryptostick->getHighwaterMarkFromSdCard(
      &HVDialog.SdCardHighWatermark_Write_Min, &HVDialog.SdCardHighWatermark_Write_Max,
      &HVDialog.SdCardHighWatermark_Read_Min, &HVDialog.SdCardHighWatermark_Read_Max);
  /*
     qDebug () << "High water mark : WriteMin WriteMax ReadMin ReadMax"; qDebug
     () << HVDialog.SdCardHighWatermark_Write_Min; qDebug () <<
     HVDialog.SdCardHighWatermark_Write_Max; qDebug () <<
     HVDialog.SdCardHighWatermark_Read_Min; qDebug () <<
     HVDialog.SdCardHighWatermark_Read_Max; */
  if (ERR_NO_ERROR != ret) {
    ret = 0; // Do something ?
  }

  HVDialog.setHighWaterMarkText();

  ret = HVDialog.exec();

  if (true == ret) {
    stick20SendCommand(STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP,
                       (unsigned char *)&HVDialog.HV_Setup_st);
  }
}

int MainWindow::UpdateDynamicMenuEntrys(void) {
  if (READ_WRITE_ACTIVE == HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8)
    NormalVolumeRWActive = FALSE;
  else
    NormalVolumeRWActive = TRUE;

  if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_CRYPTED_VOLUME_BIT_PLACE)))
    CryptedVolumeActive = TRUE;
  else
    CryptedVolumeActive = FALSE;

  if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_HIDDEN_VOLUME_BIT_PLACE)))
    HiddenVolumeActive = TRUE;
  else
    HiddenVolumeActive = FALSE;

  if (TRUE == HID_Stick20Configuration_st.StickKeysNotInitiated)
    StickNotInitated = TRUE;
  else
    StickNotInitated = FALSE;

  /*
     SDFillWithRandomChars_u8 Bit 0 = 0 SD card is *** not *** filled with
     random chars Bit 0 = 1 SD card is filled with random chars */

  if (0 == (HID_Stick20Configuration_st.SDFillWithRandomChars_u8 & 0x01))
    SdCardNotErased = TRUE;
  else
    SdCardNotErased = FALSE;
  /*
      if ((0 == HID_Stick20Configuration_st.ActiveSD_CardID_u32) || (0 ==
     HID_Stick20Configuration_st.ActiveSmartCardID_u32))
      {
          Stick20ScSdCardOnline = FALSE;  // SD card or smartcard are not ready

          if (0 == HID_Stick20Configuration_st.ActiveSD_CardID_u32)
              Stick20ActionUpdateStickStatus->setText (tr ("SD card is not
     ready"));
          if (0 == HID_Stick20Configuration_st.ActiveSmartCardID_u32)
              Stick20ActionUpdateStickStatus->setText (tr ("Smartcard is not
     ready"));
          if ((0 == HID_Stick20Configuration_st.ActiveSD_CardID_u32) && (0 ==
     HID_Stick20Configuration_st.ActiveSmartCardID_u32))
              Stick20ActionUpdateStickStatus->setText (tr ("Smartcard and SD
     card are not ready"));
      }
      else
  */
  Stick20ScSdCardOnline = TRUE;

  generateMenu();

  return (TRUE);
}

int MainWindow::stick20SendCommand(uint8_t stick20Command, uint8_t *password) {
  int ret;

  bool waitForAnswerFromStick20;

  bool stopWhenStatusOKFromStick20;

  int Result;

  QByteArray passwordString;

  waitForAnswerFromStick20 = FALSE;
  stopWhenStatusOKFromStick20 = FALSE;

  switch (stick20Command) {
  case STICK20_CMD_ENABLE_CRYPTED_PARI:
    ret = cryptostick->stick20EnableCryptedPartition(password);
    if (TRUE == ret) {
      waitForAnswerFromStick20 = TRUE;
    } else {
      csApplet->warningBox(
          tr("There was an error during communicating with device. Please try again."));
    }
    break;
  case STICK20_CMD_DISABLE_CRYPTED_PARI:
    ret = cryptostick->stick20DisableCryptedPartition();
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI:
    ret = cryptostick->stick20EnableHiddenCryptedPartition(password);
    if (TRUE == ret) {
      waitForAnswerFromStick20 = TRUE;
    } else {
      csApplet->warningBox(
          tr("There was an error during communicating with device. Please try again."));
    }
    break;
  case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI:
    ret = cryptostick->stick20DisableHiddenCryptedPartition();
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_ENABLE_FIRMWARE_UPDATE:
    ret = cryptostick->stick20EnableFirmwareUpdate(password);
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE:
    ret = cryptostick->stick20ExportFirmware(password);
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_GENERATE_NEW_KEYS:
    ret = cryptostick->stick20CreateNewKeys(password);
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS: {
    bool answer =
        csApplet->yesOrNoBox(tr("This command fills the encrypted volumes with random data "
                                "and will destroy all encrypted volumes!\n"
                                "It requires more than 1 hour for 32GB. Do you want to continue?"),
                             0, false);

    if (answer) {
      ret = cryptostick->stick20FillSDCardWithRandomChars(
          password, STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ENCRYPTED_VOL);
      if (TRUE == ret) {
        waitForAnswerFromStick20 = TRUE;
        stopWhenStatusOKFromStick20 = FALSE;
      }
    }
  } break;
  case STICK20_CMD_WRITE_STATUS_DATA:
    csApplet->messageBox(tr("Not implemented"));
    break;
  case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
    ret = cryptostick->stick20SendSetReadonlyToUncryptedVolume(password);
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
    ret = cryptostick->stick20SendSetReadwriteToUncryptedVolume(password);
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_SEND_PASSWORD_MATRIX:
    ret = cryptostick->stick20GetPasswordMatrix();
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA:
    ret = cryptostick->stick20SendPasswordMatrixPinData(password);
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_GET_DEVICE_STATUS:
    ret = cryptostick->stick20GetStatusData();
    if (TRUE == ret) {
      waitForAnswerFromStick20 = TRUE;
      stopWhenStatusOKFromStick20 = FALSE;
    }
    break;
  case STICK20_CMD_SEND_STARTUP:
    break;

  case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP:
    ret = cryptostick->stick20SendHiddenVolumeSetup((HiddenVolumeSetup_tst *)password);
    if (TRUE == ret) {
      waitForAnswerFromStick20 = TRUE;
      stopWhenStatusOKFromStick20 = FALSE;
    }
    break;

  case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND:
    ret = cryptostick->stick20SendClearNewSdCardFound(password);
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_SEND_LOCK_STICK_HARDWARE:
    ret = cryptostick->stick20LockFirmware(password);
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;
  case STICK20_CMD_PRODUCTION_TEST:
    ret = cryptostick->stick20ProductionTest();
    if (TRUE == ret)
      waitForAnswerFromStick20 = TRUE;
    break;

  default:
    csApplet->messageBox(tr("Stick20Dialog: Wrong combobox value! "));
    break;
  }

  Result = FALSE;
  if (TRUE == waitForAnswerFromStick20) {
    Stick20ResponseTask ResponseTask(this, cryptostick, trayIcon);
    if (FALSE == stopWhenStatusOKFromStick20)
      ResponseTask.NoStopWhenStatusOK();
    ResponseTask.GetResponse();
    Result = ResponseTask.ResultValue;
  }

  if (TRUE == Result) {
    switch (stick20Command) {
    case STICK20_CMD_ENABLE_CRYPTED_PARI:
      HID_Stick20Configuration_st.VolumeActiceFlag_u8 = (1 << SD_CRYPTED_VOLUME_BIT_PLACE);
      UpdateDynamicMenuEntrys();
#ifdef Q_OS_LINUX
      sleep(4); // FIXME change hard sleep to thread
      if (!QFileInfo("/dev/nitrospace").isSymLink()) {
        csApplet->warningBox(tr("Warning: The encrypted Volume is not formatted.\n\"Use GParted "
                                "or fdisk for this.\""));
      }
#endif // if Q_OS_LINUX
      break;
    case STICK20_CMD_DISABLE_CRYPTED_PARI:
      HID_Stick20Configuration_st.VolumeActiceFlag_u8 = 0;
      UpdateDynamicMenuEntrys();
      break;
    case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI:
      HID_Stick20Configuration_st.VolumeActiceFlag_u8 = (1 << SD_HIDDEN_VOLUME_BIT_PLACE);
      UpdateDynamicMenuEntrys();
      break;
    case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI:
      HID_Stick20Configuration_st.VolumeActiceFlag_u8 = 0;
      UpdateDynamicMenuEntrys();
      break;
    case STICK20_CMD_GET_DEVICE_STATUS:
      UpdateDynamicMenuEntrys();
      break;
    case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
      HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8 = READ_WRITE_ACTIVE;
      UpdateDynamicMenuEntrys();
      break;
    case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
      HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8 = READ_ONLY_ACTIVE;
      UpdateDynamicMenuEntrys();
      break;
    case STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND:
      HID_Stick20Configuration_st.SDFillWithRandomChars_u8 |= 0x01;
      UpdateDynamicMenuEntrys();
      break;
    case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS:
      HID_Stick20Configuration_st.SDFillWithRandomChars_u8 |= 0x01;
      UpdateDynamicMenuEntrys();
      break;
    case STICK20_CMD_GENERATE_NEW_KEYS: // = firmware reset
      break;
    case STICK20_CMD_PRODUCTION_TEST:
      break;
    default:
      break;
    }
  } else {
    csApplet->warningBox(tr("Either the password is not correct or the command execution resulted "
                            "in an error. Please try again."));
    return false;
  }
  return (true);
}

void MainWindow::getCode(uint8_t slotNo) {
  uint8_t result[18];

  memset(result, 0, 18);
  uint32_t code;

  cryptostick->getCode(slotNo, currentTime / 30, currentTime, 30, result);
  code = result[0] + (result[1] << 8) + (result[2] << 16) + (result[3] << 24);
  code = code % 100000000;
}

void MainWindow::on_writeButton_clicked() {
  int res;

  uint8_t SlotName[16];

  uint8_t slotNo = ui->slotComboBox->currentIndex();

  PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
                   PinDialog::ADMIN_PIN);
  bool ok;

  if (slotNo > TOTP_SlotCount)
    slotNo -= (TOTP_SlotCount + 1);
  else
    slotNo += HOTP_SlotCount;

  STRNCPY((char *)SlotName, sizeof(SlotName), ui->nameEdit->text().toLatin1(), 15);

  SlotName[15] = 0;
  if (0 == strlen((char *)SlotName)) {
    csApplet->warningBox(tr("Please enter a slotname."));
    return;
  }

  if (true == cryptostick->isConnected) {
    do {
      ui->base32RadioButton->toggle();

      if (slotNo < HOTP_SlotCount) { // HOTP slot
        HOTPSlot *hotp = new HOTPSlot();
        generateHOTPConfig(hotp);
        res = cryptostick->writeToHOTPSlot(hotp);
        delete hotp;
      } else { // TOTP slot
        TOTPSlot *totp = new TOTPSlot();
        generateTOTPConfig(totp);
        res = cryptostick->writeToTOTPSlot(totp);
        delete totp;
      }

      if (DebugingActive == TRUE) {
        QString MsgText;
        MsgText.append(tr("(debug) Response: "));
        MsgText.append(QString::number(res));
        csApplet->warningBox(MsgText);
      }

      switch (res) {
      case CMD_STATUS_OK:
        csApplet->messageBox(tr("Configuration successfully written."));
        break;
      case CMD_STATUS_NOT_AUTHORIZED:
        // Ask for password
        do {
          ok = dialog.exec();
          QString password;

          dialog.getPassword(password);

          if (QDialog::Accepted == ok) {
            uint8_t tempPassword[25];

            for (int i = 0; i < 25; i++)
              tempPassword[i] = qrand() & 0xFF;

            cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(), tempPassword);
            if (cryptostick->validPassword) {
              lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
            } else {
              csApplet->warningBox(tr("Wrong PIN. Please try again."));
            }
            password.clear();
          }
        } while (QDialog::Accepted == ok && !cryptostick->validPassword); // While
        // the user keeps enterning a pin
        // and the pin is not correct..
        break;
      case CMD_STATUS_NO_NAME_ERROR:
        csApplet->warningBox(tr("The name of the slot must not be empty."));
        break;
      default:
        QString MsgText;
        MsgText.append(tr("Error writing configuration!"));
        if (DebugingActive == TRUE) {
          MsgText.append(QString::number(res));
        }
        csApplet->warningBox(MsgText);
      }
    } while (CMD_STATUS_NOT_AUTHORIZED == res);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    Sleep::msleep(500);
    QApplication::restoreOverrideCursor();

    generateAllConfigs();
  } else
    csApplet->warningBox(tr("Nitrokey is not connected!"));

  displayCurrentSlotConfig();
}

void MainWindow::on_slotComboBox_currentIndexChanged(int index) {
  index = index; // avoid warning
  displayCurrentSlotConfig();
}

void MainWindow::on_hexRadioButton_toggled(bool checked) {
  QByteArray secret;

  uint8_t encoded[128];

  uint8_t data[128];

  uint8_t decoded[20];

  if (checked) {

    secret = ui->secretEdit->text().toLatin1();
    if (secret.size() != 0) {
      memset(encoded, 'A', 32);
      memcpy(encoded, secret.data(), secret.length());

      base32_clean(encoded, 32, data);
      base32_decode(data, decoded, 20);

      secret = QByteArray((char *)decoded, 20).toHex();

      ui->secretEdit->setText(QString(secret));
      secretInClipboard = ui->secretEdit->text();
      copyToClipboard(secretInClipboard);
    }
  }
}

void MainWindow::on_base32RadioButton_toggled(bool checked) {
  QByteArray secret;

  uint8_t encoded[128];

  uint8_t decoded[20];

  if (checked) {
    secret = QByteArray::fromHex(ui->secretEdit->text().toLatin1());
    if (secret.size() != 0) {
      memset(decoded, 0, 20);
      memcpy(decoded, secret.data(), secret.length());

      base32_encode(decoded, secret.length(), encoded, 128);

      ui->secretEdit->setText(QString((char *)encoded));
      secretInClipboard = ui->secretEdit->text();
      copyToClipboard(secretInClipboard);
    }
  }
}

void MainWindow::on_setToZeroButton_clicked() { ui->counterEdit->setText("0"); }

void MainWindow::on_setToRandomButton_clicked() {
  quint64 counter;
  counter = qrand();
  if (cryptostick->activStick20) {
    const int maxDigits = 7;
    counter = counter % ((quint64)pow(10, maxDigits));
  }
  ui->counterEdit->setText(QString(QByteArray::number(counter, 10)));
}

void MainWindow::on_tokenIDCheckBox_toggled(bool checked) {

  if (checked) {
    ui->ompEdit->setEnabled(true);
    ui->ttEdit->setEnabled(true);
    ui->muiEdit->setEnabled(true);

  } else {
    ui->ompEdit->setEnabled(false);
    ui->ttEdit->setEnabled(false);
    ui->muiEdit->setEnabled(false);
  }
}

void MainWindow::on_enableUserPasswordCheckBox_toggled(bool checked) {
  if (checked) {
    ui->deleteUserPasswordCheckBox->setEnabled(true);
    ui->deleteUserPasswordCheckBox->setChecked(false);
    cryptostick->otpPasswordConfig[0] = 1;
    cryptostick->otpPasswordConfig[1] = 0;
  } else {
    ui->deleteUserPasswordCheckBox->setEnabled(false);
    ui->deleteUserPasswordCheckBox->setChecked(false);
    cryptostick->otpPasswordConfig[0] = 0;
    cryptostick->otpPasswordConfig[1] = 0;
  }
}

void MainWindow::on_writeGeneralConfigButton_clicked() {
  int res;

  uint8_t data[5];

  PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
                   PinDialog::ADMIN_PIN);
  bool ok;

  if (cryptostick->isConnected) {

    data[0] = ui->numLockComboBox->currentIndex() - 1;
    data[1] = ui->capsLockComboBox->currentIndex() - 1;
    data[2] = ui->scrollLockComboBox->currentIndex() - 1;

    if (ui->enableUserPasswordCheckBox->isChecked()) {
      data[3] = 1;
      if (ui->deleteUserPasswordCheckBox->isChecked())
        data[4] = 1;
      else
        data[4] = 0;
    } else {
      data[3] = 0;
      data[4] = 0;
    }

    do {
      res = cryptostick->writeGeneralConfig(data);

      switch (res) {
      case CMD_STATUS_OK:
        csApplet->messageBox(tr("Configuration successfully written."));
        break;
      case CMD_STATUS_NOT_AUTHORIZED:
        // Ask for password
        do {
          ok = dialog.exec();
          QString password;

          dialog.getPassword(password);

          if (QDialog::Accepted == ok) {
            uint8_t tempPassword[25];

            for (int i = 0; i < 25; i++)
              tempPassword[i] = qrand() & 0xFF;

            cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(), tempPassword);
            if (cryptostick->validPassword) {
              lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
            } else {
              csApplet->warningBox(tr("Wrong PIN. Please try again."));
            }
            password.clear();
          }
        } while (QDialog::Accepted == ok && !cryptostick->validPassword); // While
        // the
        // user
        // keeps
        // enterning
        // a
        // pin
        // and
        // the
        // pin
        // is
        // not
        // correct..
        break;
      default:
        csApplet->warningBox(tr("Error writing configuration!"));
      }
    } while (CMD_STATUS_NOT_AUTHORIZED == res);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    Sleep::msleep(500);
    QApplication::restoreOverrideCursor();
    cryptostick->getStatus();
    generateAllConfigs();
  } else {
    csApplet->warningBox(tr("Nitrokey not connected!"));
  }
  displayCurrentGeneralConfig();
}

void MainWindow::getHOTPDialog(int slot) {
  int ret;

  ret = getNextCode(0x10 + slot);
  if (ret == 0) {
    if (cryptostick->HOTPSlots[slot]->slotName[0] == '\0')
      showTrayMessage(QString(tr("HOTP slot ")).append(QString::number(slot + 1, 10)),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
    else
      showTrayMessage(QString(tr("HOTP slot "))
                          .append(QString::number(slot + 1, 10))
                          .append(" [")
                          .append((char *)cryptostick->HOTPSlots[slot]->slotName)
                          .append("]"),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
  }
}

void MainWindow::getHOTP1() { getHOTPDialog(0); }

void MainWindow::getHOTP2() { getHOTPDialog(1); }

void MainWindow::getHOTP3() { getHOTPDialog(2); }

void MainWindow::getTOTPDialog(int slot) {
  int ret;

  ret = getNextCode(0x20 + slot);
  if (ret == 0) {
    if (cryptostick->TOTPSlots[slot]->slotName[0] == '\0')
      showTrayMessage(QString(tr("TOTP slot ")).append(QString::number(slot + 1, 10)),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
    else
      showTrayMessage(QString(tr("TOTP slot "))
                          .append(QString::number(slot + 1, 10))
                          .append(" [")
                          .append((char *)cryptostick->TOTPSlots[slot]->slotName)
                          .append("]"),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
  }
}

void MainWindow::getTOTP1() { getTOTPDialog(0); }

void MainWindow::getTOTP2() { getTOTPDialog(1); }

void MainWindow::getTOTP3() { getTOTPDialog(2); }

void MainWindow::getTOTP4() { getTOTPDialog(3); }

void MainWindow::getTOTP5() { getTOTPDialog(4); }

void MainWindow::getTOTP6() { getTOTPDialog(5); }

void MainWindow::getTOTP7() { getTOTPDialog(6); }

void MainWindow::getTOTP8() { getTOTPDialog(7); }

void MainWindow::getTOTP9() { getTOTPDialog(8); }

void MainWindow::getTOTP10() { getTOTPDialog(9); }

void MainWindow::getTOTP11() { getTOTPDialog(10); }

void MainWindow::getTOTP12() { getTOTPDialog(11); }

void MainWindow::getTOTP13() { getTOTPDialog(12); }

void MainWindow::getTOTP14() { getTOTPDialog(13); }

void MainWindow::getTOTP15() { getTOTPDialog(14); }

void MainWindow::on_eraseButton_clicked() {
  bool answer = csApplet->yesOrNoBox(tr("WARNING: Are you sure you want to erase the slot?"));
  char clean[8];

  memset(clean, ' ', 8);

  uint8_t slotNo = ui->slotComboBox->currentIndex();

  if (slotNo > TOTP_SlotCount) {
    slotNo -= (TOTP_SlotCount + 1);
  } else {
    slotNo += HOTP_SlotCount;
  }

  if (slotNo < HOTP_SlotCount) {
    memcpy(cryptostick->HOTPSlots[slotNo & 0x0F]->counter, clean, 8);
    slotNo = slotNo + 0x10;
  } else if ((slotNo >= HOTP_SlotCount) && (slotNo <= TOTP_SlotCount + HOTP_SlotCount)) {
    slotNo = slotNo + 0x20 - HOTP_SlotCount;
  }

  if (answer) {
    cryptostick->eraseSlot(slotNo);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    Sleep::msleep(1000);
    QApplication::restoreOverrideCursor();
    generateAllConfigs();

    csApplet->messageBox(tr("Slot has been erased successfully."));
  }

  displayCurrentSlotConfig();
}

void MainWindow::on_randomSecretButton_clicked() {
  int i = 0;

  uint8_t secret[32];

  char temp;

  while (i < 32) {
    temp = qrand() & 0xFF;
    if ((temp >= 'A' && temp <= 'Z') || (temp >= '2' && temp <= '7')) {
      secret[i] = temp;
      i++;
    }
  }

  QByteArray secretArray((char *)secret, 32);

  ui->base32RadioButton->setChecked(true);
  ui->secretEdit->setText(secretArray);
  ui->checkBox->setEnabled(true);
  ui->checkBox->setChecked(true);
  secretInClipboard = ui->secretEdit->text();
  copyToClipboard(secretInClipboard);
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

#include <algorithm>
void overwrite_string(QString &str) { std::fill(str.begin(), str.end(), '*'); }

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
  if (currentTime >= lastAuthenticateTime + (uint64_t)600) {
    cryptostick->validPassword = false;
    memset(cryptostick->password, 0, 25);
  }
  if (currentTime >= lastUserAuthenticateTime + (uint64_t)600 &&
      cryptostick->otpPasswordConfig[0] == 1 && cryptostick->otpPasswordConfig[1] == 1) {
    cryptostick->validUserPassword = false;
    memset(cryptostick->userPassword, 0, 25);
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
  ret = cryptostick->getPasswordSafeSlotStatus();
  if (ERR_NO_ERROR == ret) {
    PWS_Access = TRUE;
    // Setup combobox
    for (i = 0; i < PWS_SLOT_COUNT; i++) {
      if (TRUE == cryptostick->passwordSafeStatus[i]) {
        if (0 == strlen((char *)cryptostick->passwordSafeSlotNames[i])) {
          cryptostick->getPasswordSafeSlotName(i);

          STRCPY((char *)cryptostick->passwordSafeSlotNames[i],
                 sizeof(cryptostick->passwordSafeSlotNames[i]),
                 (char *)cryptostick->passwordSafeSlotName);
        }
        ui->PWS_ComboBoxSelectSlot->addItem(
            QString(tr("Slot "))
                .append(QString::number(i + 1, 10))
                .append(QString(" [")
                            .append((char *)cryptostick->passwordSafeSlotNames[i])
                            .append(QString("]"))));
      } else {
        cryptostick->passwordSafeSlotNames[i][0] = 0;
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
  int Slot;

  unsigned int ret;

  QMessageBox msgBox;

  Slot = ui->PWS_ComboBoxSelectSlot->currentIndex();

  if (0 != cryptostick->passwordSafeSlotNames[Slot][0]) // Is slot active
                                                        // ?
  {
    ret = cryptostick->passwordSafeEraseSlot(Slot);

    if (ERR_NO_ERROR == ret) {
      ui->PWS_EditSlotName->setText("");
      ui->PWS_EditPassword->setText("");
      ui->PWS_EditLoginName->setText("");
      cryptostick->passwordSafeStatus[Slot] = FALSE;
      cryptostick->passwordSafeSlotNames[Slot][0] = 0;
      ui->PWS_ComboBoxSelectSlot->setItemText(
          Slot, QString("Slot ").append(QString::number(Slot + 1, 10)));
    } else
      csApplet->warningBox(tr("Can't clear slot."));
  } else
    csApplet->messageBox(tr("Slot is erased already."));

  generateMenu();
}

void MainWindow::on_PWS_ComboBoxSelectSlot_currentIndexChanged(int index) {
  int ret;

  QString OutputText;

  if (FALSE == PWS_Access) {
    return;
  }

  // Slot already used ?
  if (TRUE == cryptostick->passwordSafeStatus[index]) {
    ui->PWS_EditSlotName->setText((char *)cryptostick->passwordSafeSlotNames[index]);

    ret = cryptostick->getPasswordSafeSlotPassword(index);
    ret = cryptostick->getPasswordSafeSlotLoginName(index);

    ui->PWS_EditPassword->setText((QString)(char *)cryptostick->passwordSafePassword);
    ui->PWS_EditLoginName->setText((QString)(char *)cryptostick->passwordSafeLoginName);
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
  int Slot;

  int ret;

  uint8_t SlotName[PWS_SLOTNAME_LENGTH + 1];

  uint8_t LoginName[PWS_LOGINNAME_LENGTH + 1];

  uint8_t Password[PWS_PASSWORD_LENGTH + 1];

  QMessageBox msgBox;

  Slot = ui->PWS_ComboBoxSelectSlot->currentIndex();

  STRNCPY((char *)SlotName, sizeof(SlotName), ui->PWS_EditSlotName->text().toLatin1(),
          PWS_SLOTNAME_LENGTH);
  SlotName[PWS_SLOTNAME_LENGTH] = 0;
  if (0 == strlen((char *)SlotName)) {
    csApplet->warningBox(tr("Please enter a slotname."));
    return;
  }

  STRNCPY((char *)LoginName, sizeof(LoginName), ui->PWS_EditLoginName->text().toLatin1(),
          PWS_LOGINNAME_LENGTH);
  LoginName[PWS_LOGINNAME_LENGTH] = 0;

  STRNCPY((char *)Password, sizeof(Password), ui->PWS_EditPassword->text().toLatin1(),
          PWS_PASSWORD_LENGTH);
  Password[PWS_PASSWORD_LENGTH] = 0;
  if (0 == strlen((char *)Password)) {
    csApplet->warningBox(tr("Please enter a password."));
    return;
  }

  ret = cryptostick->setPasswordSafeSlotData_1(Slot, (uint8_t *)SlotName, (uint8_t *)Password);
  if (ERR_NO_ERROR != ret) {
    msgBox.setText(tr("Can't save slot. %1").arg(ret));
    msgBox.exec();
    return;
  }

  ret = cryptostick->setPasswordSafeSlotData_2(Slot, (uint8_t *)LoginName);
  if (ERR_NO_ERROR != ret) {
    csApplet->warningBox(tr("Can't save slot."));
    return;
  }

  cryptostick->passwordSafeStatus[Slot] = TRUE;
  STRCPY((char *)cryptostick->passwordSafeSlotNames[Slot],
         sizeof(cryptostick->passwordSafeSlotNames[Slot]), (char *)SlotName);
  ui->PWS_ComboBoxSelectSlot->setItemText(
      Slot, QString(tr("Slot "))
                .append(QString::number(Slot + 1, 10))
                .append(QString(" [")
                            .append((char *)cryptostick->passwordSafeSlotNames[Slot])
                            .append(QString("]"))));

  generateMenu();
}

char *MainWindow::PWS_GetSlotName(int Slot) {
  if (0 == strlen((char *)cryptostick->passwordSafeSlotNames[Slot])) {
    cryptostick->getPasswordSafeSlotName(Slot);

    STRCPY((char *)cryptostick->passwordSafeSlotNames[Slot],
           sizeof(cryptostick->passwordSafeSlotNames[Slot]),
           (char *)cryptostick->passwordSafeSlotName);
  }
  return ((char *)cryptostick->passwordSafeSlotNames[Slot]);
}

void MainWindow::on_PWS_ButtonClose_pressed() { hide(); }

void MainWindow::generateMenuPasswordSafe() {
  if (FALSE == cryptostick->passwordSafeUnlocked) {
#ifdef HAVE_LIBAPPINDICATOR
    if (isUnity()) {
      GtkWidget *passwordSafeItem = gtk_image_menu_item_new_with_label(_("Unlock password safe"));
      gtk_image_menu_item_set_always_show_image(GTK_IMAGE_MENU_ITEM(passwordSafeItem), TRUE);
      GtkWidget *passwordSafeItemImg =
          gtk_image_new_from_file("/usr/share/nitrokey/safe_zahlenkreis.png");
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(passwordSafeItem),
                                    GTK_WIDGET(passwordSafeItemImg));

      g_signal_connect(passwordSafeItem, "activate", G_CALLBACK(onEnablePasswordSafe), this);
      gtk_menu_shell_append(GTK_MENU_SHELL(indicatorMenu), passwordSafeItem);
      gtk_widget_show(passwordSafeItem);
    } else
#endif // HAVE_LIBAPPINDICATOR
    {
      trayMenu->addAction(UnlockPasswordSafeAction);

      if (true == cryptostick->passwordSafeAvailable) {
        UnlockPasswordSafeAction->setEnabled(true);
      } else {
        UnlockPasswordSafeAction->setEnabled(false);
      }
    }
  }
}

void MainWindow::PWS_Clicked_EnablePWSAccess() {
  uint8_t password[LOCAL_PASSWORD_SIZE];
  bool ret;
  int ret_s32;

  QMessageBox msgBox;
  PasswordDialog dialog(FALSE, this);

  cryptostick->getUserPasswordRetryCount();
  dialog.init((char *)(tr("Enter user PIN").toUtf8().data()),
              HID_Stick20Configuration_st.UserPwRetryCount);
  dialog.cryptostick = cryptostick;

  ret = dialog.exec();

  if (QDialog::Accepted == ret) {
    dialog.getPassword((char *)password);

    ret_s32 = cryptostick->isAesSupported((uint8_t *)&password[1]);

    if (CMD_STATUS_OK == ret_s32) // AES supported, continue
    {
      cryptostick->passwordSafeAvailable = TRUE;
      UnlockPasswordSafeAction->setEnabled(true);

      // Continue to unlocking password safe
      ret_s32 = cryptostick->passwordSafeEnable((char *)&password[1]);

      if (ERR_NO_ERROR != ret_s32) {
        switch (ret_s32) {
        case CMD_STATUS_AES_DEC_FAILED:
          uint8_t admin_password[40];
          cryptostick->getUserPasswordRetryCount();
          dialog.init((char *)(tr("Enter admin PIN").toUtf8().data()),
                      HID_Stick20Configuration_st.UserPwRetryCount);
          dialog.cryptostick = cryptostick;
          ret = dialog.exec();

          if (QDialog::Accepted == ret) {
            dialog.getPassword((char *)admin_password);

            ret_s32 = cryptostick->buildAesKey((uint8_t *)&(admin_password[1]));

            if (CMD_STATUS_OK != ret_s32) {
              if (CMD_STATUS_WRONG_PASSWORD == ret_s32)
                msgBox.setText(tr("Wrong password"));
              else
                msgBox.setText(tr("Unable to create new AES key"));

              msgBox.exec();
            } else {
              Sleep::msleep(3000);
              ret_s32 = cryptostick->passwordSafeEnable((char *)&password[1]);
            }
          }

          break;
        default:
          csApplet->warningBox(tr("Can't unlock password safe."));
          break;
        }
      } else {
        PasswordSafeEnabled = TRUE;
        showTrayMessage("Nitrokey App", tr("Password Safe unlocked successfully."), INFORMATION,
                        TRAY_MSG_TIMEOUT);
        SetupPasswordSafeConfig();
        generateMenu();
        ui->tabWidget->setTabEnabled(3, 1);
      }
    } else {
      if (CMD_STATUS_NOT_SUPPORTED == ret_s32) // AES NOT supported
      {
        // Mark password safe as disabled feature
        cryptostick->passwordSafeAvailable = FALSE;
        UnlockPasswordSafeAction->setEnabled(false);
        csApplet->warningBox(tr("Password safe is not supported by this device."));
        generateMenu();
        ui->tabWidget->setTabEnabled(3, 0);
      } else {
        if (CMD_STATUS_WRONG_PASSWORD == ret_s32) // Wrong password
        {
          csApplet->warningBox(tr("Wrong user password."));
        }
      }
      return;
    }
  }
}

void MainWindow::PWS_ExceClickedSlot(int Slot) {
  QString password_safe_password;

  QString MsgText_1;

  int ret_s32;

  ret_s32 = cryptostick->getPasswordSafeSlotPassword(Slot);
  if (ERR_NO_ERROR != ret_s32) {
    csApplet->warningBox(tr("Pasword safe: Can't get password"));
    return;
  }
  password_safe_password.append((char *)cryptostick->passwordSafePassword);

  PWSInClipboard = password_safe_password;
  copyToClipboard(password_safe_password);

  memset(cryptostick->passwordSafePassword, 0, sizeof(cryptostick->passwordSafePassword));

  if (TRUE == trayIcon->supportsMessages()) {
    password_safe_password =
        QString(tr("Password safe [%1]").arg((char *)cryptostick->passwordSafeSlotNames[Slot]));
    MsgText_1 = QString("Password has been copied to clipboard");

    showTrayMessage(password_safe_password, MsgText_1, INFORMATION, TRAY_MSG_TIMEOUT);
  } else {
    password_safe_password = QString("Password safe [%1] has been copied to clipboard")
                                 .arg((char *)cryptostick->passwordSafeSlotNames[Slot]);
    csApplet->messageBox(password_safe_password);
  }
}

void MainWindow::PWS_Clicked_Slot00() { PWS_ExceClickedSlot(0); }

void MainWindow::PWS_Clicked_Slot01() { PWS_ExceClickedSlot(1); }

void MainWindow::PWS_Clicked_Slot02() { PWS_ExceClickedSlot(2); }

void MainWindow::PWS_Clicked_Slot03() { PWS_ExceClickedSlot(3); }

void MainWindow::PWS_Clicked_Slot04() { PWS_ExceClickedSlot(4); }

void MainWindow::PWS_Clicked_Slot05() { PWS_ExceClickedSlot(5); }

void MainWindow::PWS_Clicked_Slot06() { PWS_ExceClickedSlot(6); }

void MainWindow::PWS_Clicked_Slot07() { PWS_ExceClickedSlot(7); }

void MainWindow::PWS_Clicked_Slot08() { PWS_ExceClickedSlot(8); }

void MainWindow::PWS_Clicked_Slot09() { PWS_ExceClickedSlot(9); }

void MainWindow::PWS_Clicked_Slot10() { PWS_ExceClickedSlot(10); }

void MainWindow::PWS_Clicked_Slot11() { PWS_ExceClickedSlot(11); }

void MainWindow::PWS_Clicked_Slot12() { PWS_ExceClickedSlot(12); }

void MainWindow::PWS_Clicked_Slot13() { PWS_ExceClickedSlot(13); }

void MainWindow::PWS_Clicked_Slot14() { PWS_ExceClickedSlot(14); }

void MainWindow::PWS_Clicked_Slot15() { PWS_ExceClickedSlot(15); }

void MainWindow::resetTime() {
  bool ok;

  if (!cryptostick->validPassword) {

    do {
      PinDialog dialog(tr("Enter card admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
                       PinDialog::ADMIN_PIN);
      ok = dialog.exec();
      QString password;

      dialog.getPassword(password);

      if (QDialog::Accepted == ok) {
        uint8_t tempPassword[25];

        for (int i = 0; i < 25; i++)
          tempPassword[i] = qrand() & 0xFF;

        cryptostick->firstAuthenticate((uint8_t *)password.toLatin1().data(), tempPassword);
        if (cryptostick->validPassword) {
          lastAuthenticateTime = QDateTime::currentDateTime().toTime_t();
        } else {
          csApplet->warningBox(tr("Wrong Pin. Please try again."));
        }
        password.clear();
      }
    } while (QDialog::Accepted == ok && !cryptostick->validPassword);
  }

  // Start the config dialog
  if (cryptostick->validPassword) {
    cryptostick->setTime(TOTP_SET_TIME);
  }
}

int MainWindow::getNextCode(uint8_t slotNumber) {
  uint8_t result[18];

  memset(result, 0, 18);
  uint32_t code;

  uint8_t config;

  int ret;

  bool ok;

  uint16_t lastInterval = 30;

  if (cryptostick->otpPasswordConfig[0] == 1) {
    if (!cryptostick->validUserPassword) {
      cryptostick->getUserPasswordRetryCount();

      PinDialog dialog(tr("Enter card user PIN"), tr("User PIN:"), cryptostick, PinDialog::PLAIN,
                       PinDialog::USER_PIN);
      ok = dialog.exec();
      QString password;

      dialog.getPassword(password);

      if (QDialog::Accepted == ok) {
        uint8_t tempPassword[25];

        for (int i = 0; i < 25; i++)
          tempPassword[i] = qrand() & 0xFF;

        cryptostick->userAuthenticate((uint8_t *)password.toLatin1().data(), tempPassword);

        if (cryptostick->validUserPassword)
          lastUserAuthenticateTime = QDateTime::currentDateTime().toTime_t();
        password.clear();
      } else
        return 1;
    }
  }
  // Start the config dialog
  if ((TRUE == cryptostick->validUserPassword) || (cryptostick->otpPasswordConfig[0] != 1)) {

    if (slotNumber >= 0x20)
      cryptostick->TOTPSlots[slotNumber - 0x20]->interval = lastInterval;

    QString output;

    lastTOTPTime = QDateTime::currentDateTime().toTime_t();
    ret = cryptostick->setTime(TOTP_CHECK_TIME);

    bool answer;

    if (ret == -2) {
      answer = csApplet->detailedYesOrNoBox(
          tr("Time is out-of-sync"),
          tr("WARNING!\n\nThe time of your computer and Nitrokey are out of "
             "sync.\nYour computer may be configured with a wrong time "
             "or\nyour Nitrokey may have been attacked. If an attacker "
             "or\nmalware could have used your Nitrokey you should reset the "
             "secrets of your configured One Time Passwords. If your "
             "computer's time is wrong, please configure it correctly and "
             "reset the time of your Nitrokey.\n\nReset Nitrokey's time?"),
          0, false);

      if (answer) {
        resetTime();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        Sleep::msleep(1000);
        QApplication::restoreOverrideCursor();
        generateAllConfigs();
        csApplet->messageBox(tr("Time reset!"));
      } else
        return 1;
    }

    cryptostick->getCode(slotNumber, lastTOTPTime / lastInterval, lastTOTPTime, lastInterval,
                         result);
    code = result[0] + (result[1] << 8) + (result[2] << 16) + (result[3] << 24);
    config = result[4];

    if (config & (1 << 0)) {
      code = code % 100000000;
      output.append(QString("%1").arg(QString::number(code), 8, '0'));
    } else {
      code = code % 1000000;
      output.append(QString("%1").arg(QString::number(code), 6, '0'));
    }

    otpInClipboard = output;
    copyToClipboard(otpInClipboard);
    if (DebugingActive)
      qDebug() << otpInClipboard;
  } else if (ok) {
    csApplet->warningBox(tr("Invalid password!"));
    return 1;
  }

  return 0;
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
  bool conversionSuccess = false;
  ui->counterEdit->text().toLatin1().toULongLong(&conversionSuccess);
  if (cryptostick->activStick20 == false) {
    quint64 counterMaxValue = ULLONG_MAX;
    if (!conversionSuccess) {
      ui->counterEdit->setText(QString("%1").arg(0));
      csApplet->warningBox(tr("Counter must be a value between 0 and %1").arg(counterMaxValue));
    }
  } else { // for nitrokey storage
    if (!conversionSuccess || ui->counterEdit->text().toLatin1().length() > 7) {
      ui->counterEdit->setText(QString("%1").arg(0));
      csApplet->warningBox(
          tr("For Nitrokey Storage counter must be a value between 0 and 9999999"));
    }
  }
}

char *MainWindow::getFactoryResetMessage(int retCode) {
  switch (retCode) {
  case CMD_STATUS_OK:
    return strdup("Factory reset was successful.");
    break;
  case CMD_STATUS_WRONG_PASSWORD:
    return strdup("Wrong Pin. Please try again.");
    break;
  default:
    return strdup("Unknown error.");
    break;
  }
}

int MainWindow::factoryReset() {
  int ret;
  bool ok;

  do {
    PinDialog dialog(tr("Enter card admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
                     PinDialog::ADMIN_PIN);
    ok = dialog.exec();
    char password[LOCAL_PASSWORD_SIZE];
    dialog.getPassword(password);
    if (QDialog::Accepted == ok) {
      ret = cryptostick->factoryReset(password);
      csApplet->messageBox(tr(getFactoryResetMessage(ret)));
      memset(password, 0, strlen(password));
    }
  } while (QDialog::Accepted == ok && CMD_STATUS_WRONG_PASSWORD == ret);
  // Disable pwd safe menu entries
  int i;

  for (i = 0; i <= 15; i++)
    cryptostick->passwordSafeStatus[i] = false;

  // Fetch configs from device
  generateAllConfigs();
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
