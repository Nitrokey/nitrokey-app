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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QValidator>
#include <QMutex>
#include <climits>
#include "hotpslot.h"

#define TRAY_MSG_TIMEOUT 5000

namespace Ui {
class MainWindow;
}

typedef struct {
  int FlagDebug;
  int ExtendedConfigActive;
  int PasswordMatrix;
  int LockHardware;
  int Cmd;
  const char *CmdLine;
  bool language_set;
  const char *language_string;
} StartUpParameter_tst;

enum trayMessageType { INFORMATION, WARNING, CRITICAL };


class MainWindow : public QMainWindow {
  Q_OBJECT public : explicit MainWindow(StartUpParameter_tst *StartupInfo_st, QWidget *parent = 0);
  void startTimer();
  ~MainWindow();

  bool DebugWindowActive;
  bool ExtendedConfigActive;
  bool LockHardware;

  uint64_t lastTOTPTime;
  uint64_t lastClipboardTime;
  uint64_t lastAuthenticateTime;
  uint64_t lastUserAuthenticateTime;

  void resetTime();

protected:
  void closeEvent(QCloseEvent *event);

private:

    bool validate_secret(const uint8_t *secret) const;
    void initialTimeReset(int ret);
    QMutex check_connection_mutex;
  QString nkpro_user_PIN;

  void InitState();
  void createIndicator();
  void startDebug();
  void showTrayMessage(const QString &title, const QString &msg, enum trayMessageType type,
                       int timeout);

  Ui::MainWindow *ui;

  QSystemTrayIcon *trayIcon;
  QMenu *trayMenu;
  QMenu *trayMenuSubConfigure;
  QMenu *trayMenuPasswdSubMenu;
  QMenu *trayMenuTOTPSubMenu;
  QMenu *trayMenuHOTPSubMenu;
  QMenu *trayMenuSubSpecialConfigure;
  QClipboard *clipboard;

  unsigned char HOTP_SlotCount;
  unsigned char TOTP_SlotCount;

  void getSlotNames();

  int PWS_Access;
  int PWS_CreatePWSize;

  uint64_t currentTime;
  bool Stick20ScSdCardOnline;
  bool CryptedVolumeActive;
  bool PasswordSafeEnabled;
  bool HiddenVolumeActive;
  bool NormalVolumeRWActive;
  bool HiddenVolumeAccessable;
  bool StickNotInitated;
  bool SdCardNotErased;
  bool SdCardNotErased_DontAsk;
  bool StickNotInitated_DontAsk;
  bool set_initial_time;

  QAction *quitAction;
  QAction *configureAction;
  QAction *resetAction;
  QAction *configureActionStick20;
  QAction *DebugAction;
  QAction *ActionAboutDialog;
  QAction *SecPasswordAction;
  QAction *Stick20SetupAction;
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

  QString DebugText;
  QString otpInClipboard;
  QString secretInClipboard;
  QString PWSInClipboard;

  int ExecStickCmd(const char *Cmdline_);
  int getNextCode(uint8_t slotNumber);

  void generatePasswordMenu();
  void generateMenuForProDevice();
  void initActionsForStick10();
  void initActionsForStick20();
  void initCommonActions();
  int stick20SendCommand(uint8_t stick20Command, uint8_t *password);

  void generateComboBoxEntrys();
  void generateMenu();
  void generateHOTPConfig(OTPSlot *slot);
  void generateTOTPConfig(OTPSlot *slot);
  void generateAllConfigs();

  void generateMenuForStorageDevice();
  int UpdateDynamicMenuEntrys(void);
  int AnalyseProductionInfos();
  void refreshStick20StatusData();
  void translateDeviceStatusToUserMessage(const int getStatus);
  void showTrayMessage(QString message);

public slots:
  void startAboutDialog();
  void startStick10ActionChangeUserPIN();
  void startStick10ActionChangeAdminPIN();
  void startConfiguration();
  void PWS_Clicked_EnablePWSAccess();
  char *getFactoryResetMessage(int retCode);
  int factoryReset();
  void getTOTPDialog(int slot);
  void getHOTPDialog(int slot);
  void PWS_ExceClickedSlot(int Slot);
  void startStick20DestroyCryptedVolume(int fillSDWithRandomChars);
  void startStick20FillSDCardWithRandomChars();
  void startStick20EnableCryptedVolume();
  void startStick20DisableCryptedVolume();
  void startStick20EnableHiddenVolume();
  void startStick20DisableHiddenVolume();
  void startLockDeviceAction();
  void startStick20ActionChangeUserPIN();
  void startStick20ActionChangeAdminPIN();
  void startStick20SetReadOnlyUncryptedVolume();
  void startStick20SetReadWriteUncryptedVolume();
  void startStick20LockStickHardware();
  void startStick20EnableFirmwareUpdate();
  void startStick20ExportFirmwareToFile();
  void startResetUserPassword();
  void startStick20DebugAction();
  void startStick20ClearNewSdCardFound();
  void startStick20SetupHiddenVolume();

    void startStick20ActionChangeUpdatePIN();

private slots:
  void resizeMin();
  void checkConnection();
  void storage_check_symlink();

  void on_writeButton_clicked();
  void displayCurrentTotpSlotConfig(uint8_t slotNo);
  void displayCurrentHotpSlotConfig(uint8_t slotNo);
  void displayCurrentSlotConfig();
  void displayCurrentGeneralConfig();
  void on_slotComboBox_currentIndexChanged(int index);
  void on_hexRadioButton_toggled(bool checked);
  void on_base32RadioButton_toggled(bool checked);
  void on_setToZeroButton_clicked();
  void on_setToRandomButton_clicked();
  void on_tokenIDCheckBox_toggled(bool checked);
  void on_enableUserPasswordCheckBox_toggled(bool checked);
  void on_writeGeneralConfigButton_clicked();

  void copyToClipboard(QString text);
  void checkClipboard_Valid(bool force_clear = false);
  void checkPasswordTime_Valid();
  void checkTextEdited();

  // START - OTP Test Routine --------------------------------
  /*
     void on_testHOTPButton_clicked(); void on_testTOTPButton_clicked(); */
  // END - OTP Test Routine ----------------------------------

  bool eventFilter(QObject *obj, QEvent *event);
  void iconActivated(QSystemTrayIcon::ActivationReason reason);

  // Functions for password safe
  void SetupPasswordSafeConfig(void);
  void generateMenuPasswordSafe();
  char *PWS_GetSlotName(int Slot);

  void on_eraseButton_clicked();
  void on_randomSecretButton_clicked();
  void on_checkBox_toggled(bool checked);

  void startStickDebug();
  void startStick20GetStickStatus();

  void on_PWS_ButtonClearSlot_clicked();
  void on_PWS_ComboBoxSelectSlot_currentIndexChanged(int index);
  void on_PWS_CheckBoxHideSecret_toggled(bool checked);
  void on_PWS_ButtonClose_pressed();
  void on_PWS_ButtonCreatePW_clicked();
  void on_PWS_ButtonSaveSlot_clicked();
  void on_PWS_ButtonEnable_clicked();
  void on_counterEdit_editingFinished();
  void on_radioButton_2_toggled(bool checked);
  void on_radioButton_toggled(bool checked);
  void on_PWS_EditSlotName_textChanged(const QString &arg1);
  void on_PWS_EditLoginName_textChanged(const QString &arg1);
  void on_PWS_EditPassword_textChanged(const QString &arg1);

public:
  void generateTemporaryPassword(uint8_t *tempPassword) const;
  int userAuthenticate(const QString &password);
  void on_enableUserPasswordCheckBox_clicked(bool checked);
  void generateOTPConfig(OTPSlot *slot) const;
  int get_supported_secret_length_base32() const;
  int get_supported_secret_length_hex() const;
};

class utf8FieldLengthValidator : public QValidator {
  Q_OBJECT
private:
  int field_max_length;

public:
  explicit utf8FieldLengthValidator(QObject *parent = 0);
  explicit utf8FieldLengthValidator(int _field_max_length, QObject *parent = 0)
      : QValidator(parent), field_max_length(_field_max_length) {}
  virtual State validate(QString &input, int &) const {

    int chars_left = field_max_length - input.toUtf8().size();

    if (chars_left >= 0) {
      return Acceptable;
    }
    return Invalid;
  }
};

#endif // MAINWINDOW_H
