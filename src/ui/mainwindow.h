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
//#include <QSystemTrayIcon>
#include <QValidator>
#include <QMutex>
#include <climits>
#include <src/GUI/StorageActions.h>
#include "hotpslot.h"
#include "GUI/Tray.h"
#include "GUI/Clipboard.h"
#include "GUI/Authentication.h"


namespace Ui {
class MainWindow;
}

class Tray;

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


class MainWindow : public QMainWindow {
  Q_OBJECT
public :
//    explicit MainWindow(StartUpParameter_tst *StartupInfo_st, QWidget *parent = 0);
    explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

protected:
  void closeEvent(QCloseEvent *event);

private:
    Q_DISABLE_COPY(MainWindow);

    Authentication auth_admin;
    Authentication auth_user;
    StorageActions storage;
    Tray tray;
    Clipboard clipboard;

    bool validate_secret(const char *secret) const;
    void initialTimeReset(int ret);
    QMutex check_connection_mutex;
  QString nkpro_user_PIN;

  void InitState();
  void startDebug();


  Ui::MainWindow *ui;

  const unsigned char HOTP_SlotCount;
  const unsigned char TOTP_SlotCount;

  bool PWS_Access;
  int PWS_CreatePWSize;

  bool set_initial_time;


  QString DebugText;

  int ExecStickCmd(const char *Cmdline_);
  int getNextCode(uint8_t slotNumber);

  void generateHOTPConfig(OTPSlot *slot);
  void generateTOTPConfig(OTPSlot *slot);
  void generateAllConfigs();

//  void refreshStick20StatusData();
  void translateDeviceStatusToUserMessage(const int getStatus);

public slots:
  void startAboutDialog();
  void startConfiguration();
  void PWS_Clicked_EnablePWSAccess();
  char *getFactoryResetMessage(int retCode);
  int factoryReset();
  void getTOTPDialog(int slot);
  void getHOTPDialog(int slot);
  void PWS_ExceClickedSlot(int Slot);
  void startStick20ActionChangeUserPIN();
  void startStick20ActionChangeAdminPIN();
  void startStick20ActionChangeUpdatePIN();
  void startResetUserPassword();
  void startLockDeviceAction();
  void updateProgressBar(int i);

  signals:
    void PWS_unlocked();
    void PWS_slot_saved(int slot_no);
    void DeviceLocked();
    void PWS_progress(int p);
    void OTP_slot_write(int slot_no, bool isHOTP);

private slots:
  void generateComboBoxEntrys();

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

  void checkTextEdited();

  // Functions for password safe
  void SetupPasswordSafeConfig(void);

  void on_eraseButton_clicked();
  void on_randomSecretButton_clicked();
  void on_checkBox_toggled(bool checked);

  void startStickDebug();

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
  void on_enableUserPasswordCheckBox_clicked(bool checked);
  void generateOTPConfig(OTPSlot *slot) const;
  int get_supported_secret_length_base32() const;
  int get_supported_secret_length_hex() const;

};

class utf8FieldLengthValidator : public QValidator {
  Q_OBJECT
private:
  int field_max_length;
  Q_DISABLE_COPY(utf8FieldLengthValidator);


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
