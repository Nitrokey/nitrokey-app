/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *                      Parts Rudolf Boeddeker  Date: 2013-08-13
 * Copyright (c) 2012-2018 Nitrokey UG
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QValidator>
#include <QMutex>
#include <climits>
#include <src/GUI/StorageActions.h>
#include "hotpslot.h"
#include "GUI/Tray.h"
#include "GUI/Clipboard.h"
#include "GUI/Authentication.h"
#include "stick20responsedialog.h"
#include "stick20debugdialog.h"
#include <atomic>


namespace Ui {
class MainWindow;
}

class Tray;

enum class ConnectionState{
  disconnected, connected, long_operation
};

class MainWindow : public QMainWindow {
  Q_OBJECT

public :
    explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

protected:
  void closeEvent(QCloseEvent *event) override;
  void showEvent(QShowEvent *event) override;

private:
  Q_DISABLE_COPY(MainWindow);

  Ui::MainWindow *ui;
  Clipboard clipboard;
  Authentication auth_admin;
  Authentication auth_user;
  StorageActions storage;
  Tray tray;
  const unsigned char HOTP_SlotCount;
  const unsigned char TOTP_SlotCount;
  DebugDialog *debug;

  void keyPressEvent(QKeyEvent *keyevent) override;

  bool validate_raw_secret(const char *secret) const;
  void initialTimeReset();
  QMutex check_connection_mutex;
  QString nkpro_user_PIN;
  void startDebug();
  QTimer *keepDeviceOnlineTimer;


  bool PWS_Access = false;
  const int PWS_CreatePWSize = 20;

  bool set_initial_time;


  QString DebugText;

  int ExecStickCmd(const char *Cmdline_);
  std::string getNextCode(uint8_t slotNumber);

  void generateHOTPConfig(OTPSlot *slot);
  void generateTOTPConfig(OTPSlot *slot);
  void generateAllConfigs();

//  void refreshStick20StatusData();
  void translateDeviceStatusToUserMessage(const int getStatus);

public slots:
  void startAboutDialog();
  void startHelpAction();
  void startConfiguration(bool changeTab = true);
  void startConfigurationMain();
  void PWS_Clicked_EnablePWSAccess();

  int factoryResetAction();
  void getTOTPDialog(int slot);
  void getHOTPDialog(int slot);
  void PWS_ExceClickedSlot(int Slot);
  void startStick20ActionChangeUserPIN();
  void startStick20ActionChangeAdminPIN();
  void startStick20ActionChangeUpdatePIN();
  void startResetUserPassword();
  void startLockDeviceAction(bool ask_for_confirmation = true);
  void updateProgressBar(int i);
  void show_progress_window();

signals:
  void DeviceConnected();
  void DeviceDisconnected();
  void PWS_unlocked();
  void PWS_slot_saved(int slot_no);
  void DeviceLocked();
  void FactoryReset();
  void PWS_progress(int p);
  void OperationInProgress(int p);
  void ShortOperationBegins(QString msg);
  void ShortOperationEnds();
  void OTP_slot_write(int slot_no, bool isHOTP);
  void DebugData(QString msg);
  void LongOperationStart();

private slots:
  void manageStartPage();
  void on_longOperationStart();
  void on_KeepDeviceOnline();
  void on_DeviceConnected();
  void on_DeviceDisconnected();
  void generateComboBoxEntrys();
  void on_enableUserPasswordCheckBox_clicked(bool checked);

  void resizeMin();
  void ready();
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
  void on_enableUserPasswordCheckBox_toggled(bool checked);
  void on_writeGeneralConfigButton_clicked();

  void checkTextEdited();

  // Functions for password safe
  void SetupPasswordSafeConfig(void);

  void on_eraseButton_clicked();
  void on_randomSecretButton_clicked();
  void on_checkBox_toggled(bool checked);

  void startStickDebug();
  void load_settings_page();

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


  void on_btn_writeSettings_clicked();

  void on_btn_select_debug_file_path_clicked();

  void on_PWS_Lock_clicked();

  void on_btn_copyToClipboard_clicked();

  void on_btn_select_debug_console_clicked();

public:
  void generateOTPConfig(OTPSlot *slot);
  unsigned int get_supported_secret_length_base32() const;
  unsigned int get_supported_secret_length_hex() const;

  std::atomic_bool long_operation_in_progress {false};
  std::shared_ptr<Stick20ResponseDialog> progress_window;

  void PWS_set_controls_enabled(bool enabled) const;

  ConnectionState connectionState = ConnectionState::disconnected;

  void set_commands_delay(int delay_in_ms);

  void first_run();
  void enable_admin_commands();
  void set_debug_file(QString log_file_name);
  void set_debug_window();

  void set_debug_mode();
  void set_debug_level(int debug_level);

  void hideOnStartup();

private:
  bool debug_mode = false;
  bool suppress_next_show = false;

  void showNotificationLabel();

  unsigned int roundToNextMultiple(const int number, const int multipleOf) const;

  QString getOTPSecretCleaned(QString secret_input);

  void make_UI_enabled(bool enabled);

  void check_libnitrokey_version();

};

quint32 get_random();

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
