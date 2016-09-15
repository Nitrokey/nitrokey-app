/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
 *
 * This file is part of Nitrokey 2
 *
 * Nitrokey 2  is free software: you can redistribute it and/or modify
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

#include "stick20-response-task.h"
#include "stick20responsedialog.h"

#ifdef HAVE_LIBAPPINDICATOR
#undef signals
extern "C" {
#include <libnotify/notify.h>
}
#define signals public
#endif // HAVE_LIBAPPINDICATOR

// #include "ui_stick20-response-task.h"

class OwnSleep : public QThread {
public:
  static void usleep(unsigned long usecs) { QThread::usleep(usecs); }
  static void msleep(unsigned long msecs) { QThread::msleep(msecs); }
  static void sleep(unsigned long secs) { QThread::sleep(secs); }
};

Stick20ResponseTask::Stick20ResponseTask(QWidget *parent, Device *Cryptostick20,
                                         QSystemTrayIcon *MainWndTrayIcon) {
  ActiveCommand = -1;
  EndFlag = FALSE;
  FlagNoStopWhenStatusOK = FALSE;
  ResultValue = FALSE;
  Counter_u32 = 0;
  retStick20Respone = 0;

  Stick20ResponseTaskParent = parent;

  cryptostick = Cryptostick20;
  trayIcon = MainWndTrayIcon;

  stick20Response = new Response();
}

void Stick20ResponseTask::NoStopWhenStatusOK() { FlagNoStopWhenStatusOK = TRUE; }

bool isUnity() {
  QString desktop = getenv("XDG_CURRENT_DESKTOP");

  return (desktop.toLower() == "unity" || desktop.toLower() == "kde" ||
          desktop.toLower() == "lxde" || desktop.toLower() == "xfce");
}

void Stick20ResponseTask::ShowIconMessage(const QString msg) {
  QString title = QString("Nitrokey App");
  int timeout = 3000;
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
      trayIcon->showMessage(title, msg, QSystemTrayIcon::Information, timeout);
    } else
      csApplet()->messageBox(msg);
  }
}

/*
   void Stick20ResponseTask::ShowIconMessage (QString IconText) { if (TRUE ==
   trayIcon->supportsMessages ()) { trayIcon->showMessage ("Nitrokey App",
   IconText); } else { QMessageBox msgBox;

   msgBox.setText (IconText); msgBox.exec (); } } */

#define RESPONSE_DIALOG_TIME_TO_SHOW_DIALOG 30 // a 100 ms = 3 sec

void Stick20ResponseTask::checkStick20Status() {
  QString OutputText;

  Counter_u32++;

  retStick20Respone = stick20Response->getResponse(cryptostick);

  if (0 == retStick20Respone) {
    if (-1 == ActiveCommand) {
      ActiveCommand = stick20Response->HID_Stick20Status_st.LastCommand_u8;
    }

    switch (stick20Response->HID_Stick20Status_st.Status_u8) {
    case OUTPUT_CMD_STICK20_STATUS_IDLE:
      break;
    case OUTPUT_CMD_STICK20_STATUS_OK:
      EndFlag = TRUE;
      break;
    case OUTPUT_CMD_STICK20_STATUS_BUSY:
      break;
    case OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD:
      switch (ActiveCommand) { csApplet()->warningBox(tr("Wrong password")); }
      EndFlag = TRUE;
      break;
    case OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR:
      break;
    case OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY:
      EndFlag = TRUE;
      break;
    case OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK:
      EndFlag = TRUE;
      break;
    case OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR:
      EndFlag = TRUE;
      break;
    case OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE:
      EndFlag = TRUE;
      break;
    default:
      break;
    }
    if (TRUE == FlagNoStopWhenStatusOK) {
      switch (stick20Response->HID_Stick20Status_st.Status_u8) {
      case OUTPUT_CMD_STICK20_STATUS_OK:
        done(TRUE);
        ResultValue = TRUE;
        break;
      case OUTPUT_CMD_STICK20_STATUS_IDLE:
      case OUTPUT_CMD_STICK20_STATUS_BUSY:
      case OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR:
      case OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY:
        // Do nothing, wait for next hid info
        break;
      case OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD:
        done(FALSE);
        ResultValue = FALSE;
        switch (ActiveCommand) {
        case STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED:
        case STICK20_CMD_SEND_LOCK_STICK_HARDWARE:
        case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE:
        case STICK20_CMD_GENERATE_NEW_KEYS:
        case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS:
          if (0 < HID_Stick20Configuration_st.AdminPwRetryCount) {
            HID_Stick20Configuration_st.AdminPwRetryCount--;
          }
          break;
        case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
        case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
        case STICK20_CMD_ENABLE_CRYPTED_PARI:
          if (0 < HID_Stick20Configuration_st.UserPwRetryCount) {
            HID_Stick20Configuration_st.UserPwRetryCount--;
          }
          break;
        }
        break;
      case OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK:
        done(FALSE);
        ResultValue = FALSE;
        break;
      case OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR:
        done(FALSE);
        ResultValue = FALSE;
        break;
      case OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE:
        done(FALSE);
        ResultValue = FALSE;
        break;
      }
    }

    if (OUTPUT_CMD_STICK20_STATUS_OK == stick20Response->HID_Stick20Status_st.Status_u8) {
      ResultValue = TRUE;

      switch (ActiveCommand) {
      case STICK20_CMD_ENABLE_CRYPTED_PARI:
        ShowIconMessage(tr("Encrypted volume unlocked successfully"));
        HID_Stick20Configuration_st.UserPwRetryCount = 3;
        break;
      case STICK20_CMD_DISABLE_CRYPTED_PARI:
        ShowIconMessage(tr("Encrypted volume locked successfully"));
        break;
      case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI:
        ShowIconMessage(tr("Hidden volume unlocked successfully"));
        break;
      case STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI:
        ShowIconMessage(tr("Hidden volume locked successfully"));
        break;
      case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP:
        ShowIconMessage(tr("Hidden volume setup successfully"));
        break;
      case STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN:
        ShowIconMessage(tr("Cleartext volume is in readonly mode"));
        HID_Stick20Configuration_st.UserPwRetryCount = 3;
        break;
      case STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN:
        ShowIconMessage(tr("Cleartext volume is in readwrite mode"));
        HID_Stick20Configuration_st.UserPwRetryCount = 3;
        break;
      case STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED:
        ShowIconMessage(tr("Warning disabled"));
        HID_Stick20Configuration_st.AdminPwRetryCount = 3;
        break;
      case STICK20_CMD_SEND_LOCK_STICK_HARDWARE:
        ShowIconMessage(tr("Firmware is locked"));
        HID_Stick20Configuration_st.AdminPwRetryCount = 3;
        break;
      case STICK20_CMD_EXPORT_FIRMWARE_TO_FILE:
        ShowIconMessage(tr("Firmware exported successfully"));
        HID_Stick20Configuration_st.AdminPwRetryCount = 3;
        break;
      case STICK20_CMD_GENERATE_NEW_KEYS:
        ShowIconMessage(tr("New keys generated successfully"));
        HID_Stick20Configuration_st.AdminPwRetryCount = 3;
        break;
      case STICK20_CMD_GET_DEVICE_STATUS:
        // showStick20Configuration (ret);
        break;
      case STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS:
        HID_Stick20Configuration_st.AdminPwRetryCount = 3;
        { csApplet()->messageBox(tr("Storage successfully initialized with random data")); }
        done(TRUE);
        break;
      default:
        break;
      }
    }

    if (OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD ==
        stick20Response->HID_Stick20Status_st.Status_u8) {
      switch (ActiveCommand) {
      case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI: {
        csApplet()->warningBox(tr("Can't enable hidden volume"));
      } break;
      default:
        break;
      }
    }

    if (OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK ==
        stick20Response->HID_Stick20Status_st.Status_u8) {
      switch (ActiveCommand) {
      case STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP: {
        // msgBox.setText("To setup the hidden volume, please
        // enable the encrypted volume to enable smartcard
        // access");
        csApplet()->warningBox(tr("Please enable the encrypted volume first."));
      } break;
      case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI: {
        csApplet()->messageBox(tr("Encrypted volume was not enabled, please "
                                "enable the encrypted volume"));
      } break;
      default:
        break;
      }
    }

    if (OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR ==
        stick20Response->HID_Stick20Status_st.Status_u8) {
      switch (ActiveCommand) {
      case STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI: {
        csApplet()->warningBox(tr("Smartcard error, please retry the command"));
      } break;
      default:
        break;
      }
    }

    if (OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE ==
        stick20Response->HID_Stick20Status_st.Status_u8) {
      switch (ActiveCommand) {
      case STICK20_CMD_ENABLE_FIRMWARE_UPDATE: {
        csApplet()->messageBox(tr("Security bit of the device is activated.\nFirmware update is "
                                "not possible."));
      } break;
      default:
        break;
      }
    }
  }
}

void Stick20ResponseTask::done(int Status) { EndFlag = TRUE; }

void Stick20ResponseTask::GetResponse(void) {
  int i;

  for (i = 0; i < 15; i++) {
    OwnSleep::msleep(100);
    checkStick20Status();
    if (TRUE == EndFlag) {
      return;
    }
  }

  if (FALSE == EndFlag) {
    Stick20ResponseDialog ResponseDialog(Stick20ResponseTaskParent, this);

    // ResponseDialog.Stick20Task = this;
    ResponseDialog.exec();
  }
}

Stick20ResponseTask::~Stick20ResponseTask() { delete stick20Response; }
