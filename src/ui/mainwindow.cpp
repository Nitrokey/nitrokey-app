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

#include <libnitrokey/NitrokeyManager.h>

#include "mainwindow.h"
#include "aboutdialog.h"
#include "pindialog.h"
#include "stick20debugdialog.h"
#include "ui_mainwindow.h"

#include "nitrokey-applet.h"
#include "securitydialog.h"
#include "stick20changepassworddialog.h"
#include "stick20hiddenvolumedialog.h"
#include "stick20lockfirmwaredialog.h"
#include "stick20responsedialog.h"
#include "libada.h"

#include <QDateTime>
#include <QDialog>
#include <QMenu>
#include <QThread>
#include <QTimer>
#include <QtWidgets>

#include "src/core/SecureString.h"

#include <QString>
#include <src/core/ScopedGuard.h>

using nm = nitrokey::NitrokeyManager;
static const QString communication_error_message = QApplication::tr("Communication error. Please reinsert the device.");


static const QString invalid_secret_key_string_details = QApplication::tr("The secret string you have entered is invalid. Please reenter it.")
                           +"\n"+QApplication::tr("Details: ");

const QString MainWindow::RESET_NITROKEYS_TIME = MainWindow::tr("Reset Nitrokey's time?");
const QString MainWindow::WARNING_DEVICES_CLOCK_NOT_DESYNCHRONIZED =
    MainWindow::tr("WARNING!\n\nThe time of your computer and Nitrokey are out of "
                         "sync. Your computer may be configured with a wrong time or "
                         "your Nitrokey may have been attacked. If an attacker or "
                         "malware could have used your Nitrokey you should reset the "
                         "secrets of your configured One Time Passwords. If your "
                         "computer's time is wrong, please configure it correctly and "
                         "reset the time of your Nitrokey.\n\nReset Nitrokey's time?");

const auto tray_location_msg = "\n"+MainWindow::tr("You can find application’s tray icon in system tray in "
                                      "the right down corner of your screen (Windows) or in the upper right (Linux, MacOS).");

void MainWindow::load_settings(){
    QSettings settings;
    const auto first_run_key = "main/first_run";
    auto first_run = settings.value(first_run_key, true).toBool();
    ui->cb_first_run_message->setChecked(first_run);

    const auto lang_key = "main/language";
    auto lang_selected = settings.value(lang_key, "-----").toString();

    ui->combo_languages->clear();
    QDir langDir(":/i18n/");
    auto list = langDir.entryList();
    for (auto &&translationFile : list) {
      auto lang = translationFile.remove("nitrokey_").remove(".qm");
      ui->combo_languages->addItem(lang, lang);
    }
    ui->combo_languages->addItem("current: " + lang_selected, lang_selected);
    ui->combo_languages->setCurrentText("current: " + lang_selected);

    ui->edit_debug_file_path->setText(settings.value("debug/file", "").toString());
    ui->spin_debug_verbosity->setValue(settings.value("debug/level", 2).toInt());
    ui->cb_debug_enabled->setChecked(settings.value("debug/enabled", false).toBool());
    emit ui->cb_debug_enabled->toggled(settings.value("debug/enabled", false).toBool());
    ui->spin_PWS_time->setValue(settings.value("clipboard/PWS_time", 60).toInt());
    ui->spin_OTP_time->setValue(settings.value("clipboard/OTP_time", 120).toInt());
    ui->cb_device_connection_message->setChecked(settings.value("main/connection_message", true).toBool());
    ui->cb_check_symlink->setChecked(settings.value("storage/check_symlink", true).toBool());
#ifndef Q_OS_LINUX
    ui->cb_check_symlink->setEnabled(false);
#endif
}

void MainWindow::keyPressEvent(QKeyEvent *keyevent)
{
    if (keyevent->key()==Qt::Key_Escape){
         hide();
    } else {
        QMainWindow::keyPressEvent(keyevent);
    }
}

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    clipboard(this),
    auth_admin(this, Authentication::Type::ADMIN),
    auth_user(this, Authentication::Type::USER),
    storage(this, &auth_admin, &auth_user),
    tray(this, false, false, &storage),
    HOTP_SlotCount(HOTP_SLOT_COUNT),
    TOTP_SlotCount(TOTP_SLOT_COUNT)
{
  debug = new DebugDialog(this);
  connect(this, SIGNAL(DebugData(QString)), debug, SLOT(on_DebugData(QString)));

  progress_window = std::make_shared<Stick20ResponseDialog>();
  //TODO move from functors to signals
  storage.set_start_progress_window( [this](QString msg){ emit ShortOperationBegins(msg); });
  storage.set_end_progress_window([this](){ emit ShortOperationEnds(); });
  storage.set_show_message( [this](QString msg){ tray.showTrayMessage(msg); });

  //TODO make connections in objects instead of accumulating them here
//  connect(&storage, SIGNAL(storageStatusChanged()), &tray, SLOT(regenerateMenu()));
  connect(&storage, SIGNAL(storageStatusUpdated()), &tray, SLOT(regenerateMenu()));
  connect(&storage, SIGNAL(FactoryReset()), &tray, SLOT(regenerateMenu()));
  connect(&storage, SIGNAL(FactoryReset()), libada::i().get(), SLOT(on_FactoryReset()));
  connect(&storage, SIGNAL(longOperationStarted()), this, SLOT(on_KeepDeviceOnline()));
  connect(this, SIGNAL(FactoryReset()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(FactoryReset()), libada::i().get(), SLOT(on_FactoryReset()));

  connect(this, SIGNAL(PWS_unlocked()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(PWS_unlocked()), this, SLOT(SetupPasswordSafeConfig()));
  connect(this, SIGNAL(DeviceLocked()), this, SLOT(SetupPasswordSafeConfig()));
  connect(&storage, SIGNAL(storageStatusChanged()), this, SLOT(SetupPasswordSafeConfig()));
  connect(&storage, SIGNAL(storageStatusChanged()), this, SLOT(storage_check_symlink()));
  connect(this, SIGNAL(PWS_slot_saved(int)), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(PWS_slot_saved(int)), libada::i().get(), SLOT(on_PWS_save(int)));

  connect(this, SIGNAL(OTP_slot_write(int, bool)), libada::i().get(), SLOT(on_OTP_save(int, bool)));
  connect(libada::i().get(), SIGNAL(regenerateMenu()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(DeviceLocked()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(DeviceLocked()), &storage, SLOT(on_StorageStatusChanged()));
  connect(this, SIGNAL(DeviceConnected()), &storage, SLOT(on_StorageStatusChanged()));
  connect(&tray, SIGNAL(progress(int)), this, SLOT(updateProgressBar(int)));
  connect(this, SIGNAL(ShortOperationBegins(QString)), progress_window.get(), SLOT(on_ShortOperationBegins(QString)));
  connect(this, SIGNAL(ShortOperationEnds()), progress_window.get(), SLOT(on_ShortOperationEnds()));
  connect(this, SIGNAL(OperationInProgress(int)), &tray, SLOT(updateOperationInProgressBar(int)));
  connect(this, SIGNAL(OperationInProgress(int)), progress_window.get(), SLOT(updateOperationInProgressBar(int)));
  connect(this, SIGNAL(DeviceDisconnected()), this, SLOT(on_DeviceDisconnected()));
  connect(this, SIGNAL(DeviceDisconnected()), libada::i().get(), SLOT(on_DeviceDisconnect()));
  connect(this, SIGNAL(DeviceConnected()), this, SLOT(on_DeviceConnected()));
  connect(this, SIGNAL(DeviceConnected()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(DeviceDisconnected()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(DeviceConnected()), &auth_admin, SLOT(clearTemporaryPasswordForced()));
  connect(this, SIGNAL(DeviceConnected()), &auth_user, SLOT(clearTemporaryPasswordForced()));
  connect(this, SIGNAL(DeviceLocked()), &auth_admin, SLOT(clearTemporaryPasswordForced()));
  connect(this, SIGNAL(DeviceLocked()), &auth_user, SLOT(clearTemporaryPasswordForced()));

  ui->setupUi(this);
  ui->tabWidget->setCurrentIndex(0); // Set first tab active
  PWS_set_controls_enabled(false);
  ui->PWS_ButtonCreatePW->setText(QString(tr("Generate random password ")));
  ui->PWS_progressBar->hide();
  ui->statusBar->showMessage(tr("Nitrokey disconnected"));
  ui->l_supportedLength->hide();

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
  timer->start(2000);
  QTimer::singleShot(500, this, SLOT(checkConnection()));

  keepDeviceOnlineTimer = new QTimer(this);
  connect(keepDeviceOnlineTimer, SIGNAL(timeout()), this, SLOT(on_KeepDeviceOnline()));
  keepDeviceOnlineTimer->start(30*1000);
  QTimer::singleShot(10*1000, this, SLOT(on_KeepDeviceOnline()));


  connect(ui->secretEdit, SIGNAL(textEdited(QString)), this, SLOT(checkTextEdited()));

  ui->deleteUserPasswordCheckBox->setEnabled(false);
  ui->deleteUserPasswordCheckBox->setChecked(false);

  ui->PWS_EditPassword->setValidator(
      new utf8FieldLengthValidator(PWS_PASSWORD_LENGTH, ui->PWS_EditPassword));
  ui->PWS_EditLoginName->setValidator(
      new utf8FieldLengthValidator(PWS_LOGINNAME_LENGTH, ui->PWS_EditLoginName));
  ui->PWS_EditSlotName->setValidator(
      new utf8FieldLengthValidator(PWS_SLOTNAME_LENGTH, ui->PWS_EditSlotName));

  this->adjustSize();
  this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());

  first_run();

  load_settings();
}

void MainWindow::first_run(){
  QSettings settings;
  const auto first_run_key = "main/first_run";
  auto first_run = settings.value(first_run_key, true).toBool();
  if (!first_run) return;

  auto msg = tr("The Nitrokey App is available as an icon in the tray bar.");
  tray.showTrayMessage(msg);
  msg += tray_location_msg;
  msg += " " + tr("Would you like to show this message again?");
  bool user_wants_to_be_reminded = csApplet()->yesOrNoBox(msg, true);
  if(!user_wants_to_be_reminded){
      settings.setValue(first_run_key, false);
  }

  //TODO insert call to First run configuration wizard here
}

void MainWindow::checkConnection() {
  using cs = ConnectionState;

  if (!check_connection_mutex.tryLock(100)){
    if (debug_mode)
      qDebug("checkConnection skip");
    return;
  }

  ScopedGuard mutexGuard([this](){
      check_connection_mutex.unlock();
  });

  bool deviceConnected = libada::i()->isDeviceConnected() && !libada::i()->have_communication_issues_occurred();

  static std::atomic_int connection_trials {0};

  if(!deviceConnected && nm::instance()->could_current_device_be_enumerated()){
    connection_trials++;
    if(connection_trials%5==0){
      csApplet()->warningBox(
          tr("Device lock detected, please remove and insert the device again.\nIf problem will occur again please: \n1. Close the application\n2. Reinsert the device\n3. Wait 30 seconds and start application")
      );
    }
  }

  if (deviceConnected){
        connection_trials = std::max(0, static_cast<int>(connection_trials )- 1);
          if(connectionState == cs::disconnected){
              connectionState = cs::connected;
              if (debug_mode)
                emit ShortOperationBegins("Connecting Device");

              QTimer::singleShot(3000, [this](){
                  emit ShortOperationEnds();
                  nitrokey::NitrokeyManager::instance()->connect();

              //on connection
                  emit DeviceConnected();
              });
          }

  } else { //device not connected
      if(connectionState == cs::connected){
          connectionState = cs::disconnected;
      //on disconnection
      emit DeviceDisconnected();
    }
    nitrokey::NitrokeyManager::instance()->connect();
  }
}

void MainWindow::initialTimeReset() {
  if (!libada::i()->isDeviceConnected()) {
    return;
  }

  if (!libada::i()->is_time_synchronized()) {
    bool answer = csApplet()->detailedYesOrNoBox(tr("Time is out-of-sync") + " - " + RESET_NITROKEYS_TIME, WARNING_DEVICES_CLOCK_NOT_DESYNCHRONIZED,
      false);
    if (answer) {
      auto res = libada::i()->set_current_time();
      if (res) {
          tray.showTrayMessage(tr("Time reset!"));
      }
    }
  }
}

MainWindow::~MainWindow() {
  nm::instance()->set_log_function([](std::string /*data*/){});
  delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
  this->hide();
  event->ignore();
}

void MainWindow::generateComboBoxEntrys() {
  //FIXME run in separate thread
  int i;

  ui->slotComboBox->clear();

  for (i = 0; i < TOTP_SlotCount; i++) {
    auto slotName = libada::i()->getTOTPSlotName(i);
    if (slotName.empty())
      ui->slotComboBox->addItem(QString(tr("TOTP slot ")).append(QString::number(i + 1, 10)));
    else
      ui->slotComboBox->addItem(QString(tr("TOTP slot "))
                                    .append(QString::number(i + 1, 10))
                                    .append(" [")
                                    .append(QString::fromStdString(slotName))
                                    .append("]"));
  }

  ui->slotComboBox->insertSeparator(TOTP_SlotCount + 1);

  for (i = 0; i < HOTP_SlotCount; i++) {
    auto slotName = libada::i()->getHOTPSlotName(i);
    if (slotName.empty())
      ui->slotComboBox->addItem(QString(tr("HOTP slot ")).append(QString::number(i + 1, 10)));
    else
      ui->slotComboBox->addItem(QString(tr("HOTP slot "))
                                    .append(QString::number(i + 1, 10))
                                    .append(" [")
                                    .append(QString::fromStdString(slotName))
                                    .append("]"));
  }

  ui->slotComboBox->setCurrentIndex(0);
}



void MainWindow::generateHOTPConfig(OTPSlot *slot) {
  slot->type = OTPSlot::OTPType::HOTP;

  uint8_t selectedSlot = ui->slotComboBox->currentIndex();
  selectedSlot -= (TOTP_SlotCount + 1);

  if (selectedSlot < HOTP_SlotCount) {
    slot->slotNumber = selectedSlot;// + 0x10;

    generateOTPConfig(slot);

    memset(slot->counter, 0, 8);
    // Nitrokey Storage needs counter value in text but Pro in binary [#60]
      bool conversionSuccess = false;
      uint64_t counterFromGUI = 0;
      if (0 != ui->counterEdit->text().toLatin1().length()) {
        counterFromGUI = ui->counterEdit->text().toLatin1().toULongLong(&conversionSuccess);
      }
      if (conversionSuccess) {
        memcpy(slot->counter, &counterFromGUI, sizeof counterFromGUI);
      } else {
          csApplet()->warningBox(tr("Counter value not copied - there was an error in conversion. "
                                            "Setting counter value to 0. Please retry."));
      }
    }
}

#include <src/GUI/ManageWindow.h>

#include <cppcodec/hex_upper.hpp>


void MainWindow::generateOTPConfig(OTPSlot *slot) {
  using hex = cppcodec::hex_upper;

  std::string secret_hex;
  ui->base32RadioButton->toggle();
  auto cleaned = getOTPSecretCleaned(ui->secretEdit->text());
  ui->secretEdit->setText(cleaned);
  auto secretFromGUI = cleaned.toLatin1().toStdString();

  if(ui->base32RadioButton->isChecked()){
    auto secret_raw = decodeBase32Secret(secretFromGUI, debug_mode);
    secret_hex = hex::encode(secret_raw);
  } else{
    secret_hex = secretFromGUI;
  }

  size_t toCopy = std::min(sizeof(slot->secret)-1, (const size_t &) secret_hex.length());
  if (!libada::i()->is_secret320_supported() && toCopy > 40){
    toCopy = 40;
  }
  memset(slot->secret, 0, sizeof(slot->secret));
  std::copy(secret_hex.begin(), secret_hex.begin()+toCopy, slot->secret);

  //TODO to rewrite
  QByteArray slotNameFromGUI = QByteArray(this->ui->nameEdit->text().toLatin1());
  memset(slot->slotName, 0, sizeof(slot->slotName));
  toCopy = std::min(sizeof(slot->slotName)-1, (const size_t &) slotNameFromGUI.length());
  memcpy(slot->slotName, slotNameFromGUI.constData(), toCopy);

  memset(slot->tokenID, 0, sizeof(slot->tokenID));
  QByteArray ompFromGUI = (this->ui->ompEdit->text().toLatin1());
  toCopy = std::min(2ul, (const unsigned long &) ompFromGUI.length());
  memcpy(slot->tokenID, ompFromGUI.constData(), toCopy);

  QByteArray ttFromGUI = (this->ui->ttEdit->text().toLatin1());
  toCopy = std::min(2ul, (const unsigned long &) ttFromGUI.length());
  memcpy(slot->tokenID + 2, ttFromGUI.constData(), toCopy);

  QByteArray muiFromGUI = (this->ui->muiEdit->text().toLatin1());
  toCopy = std::min(8ul, (const unsigned long &) muiFromGUI.length());
  memcpy(slot->tokenID + 4, muiFromGUI.constData(), toCopy);

//  slot->tokenID[12] = (uint8_t) (this->ui->keyboardComboBox->currentIndex() & 0xFF);

  slot->config = 0;
  if (ui->digits8radioButton->isChecked())
      slot->config += (1 << 0);
  if (ui->enterCheckBox->isChecked())
      slot->config += (1 << 1);
  if (ui->tokenIDCheckBox->isChecked())
      slot->config += (1 << 2);
}

void MainWindow::generateTOTPConfig(OTPSlot *slot) {
  slot->type = OTPSlot::OTPType::TOTP;

  uint8_t selectedSlot = ui->slotComboBox->currentIndex();

  // get the TOTP slot number
  if (selectedSlot < TOTP_SlotCount) {
    slot->slotNumber = selectedSlot;// + 0x20;

    generateOTPConfig(slot);

    uint16_t lastInterval = ui->intervalSpinBox->value();

    if (lastInterval < 1)
      lastInterval = 1;

    slot->interval = lastInterval;
  }
}

void MainWindow::generateAllConfigs() {
  displayCurrentSlotConfig();
//  generateMenu(false);
}

void updateSlotConfig(const nitrokey::ReadSlot::ResponsePayload &p, Ui::MainWindow* ui)  {
  ui->ompEdit->setText(QString(reinterpret_cast<const char*>(p.slot_token_fields.omp)).trimmed());
  ui->ttEdit->setText(QString(reinterpret_cast<const char*>(p.slot_token_fields.tt)).trimmed());
  ui->muiEdit->setText(QString(reinterpret_cast<const char*>(p.slot_token_fields.mui)).trimmed());

  if (p.use_8_digits)
    ui->digits8radioButton->setChecked(true);
  else
    ui->digits6radioButton->setChecked(true);

  ui->enterCheckBox->setChecked(p.use_enter);
  ui->tokenIDCheckBox->setChecked(p.use_tokenID);
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
  ui->secretEdit->clear();
  ui->secretEdit->setPlaceholderText("********************************");

  ui->nameEdit->setText(QString::fromStdString(libada::i()->getTOTPSlotName(slotNo)));
  ui->base32RadioButton->setChecked(true);

  ui->counterEdit->setText("0");
  ui->tokenIDCheckBox->setChecked(false);
  ui->digits6radioButton->setChecked(true);

  ui->ompEdit->setText("NK");
  ui->ttEdit->setText("01");
  std::string cardSerial = libada::i()->get_serial_number();
  ui->muiEdit->setText(QString("%1").arg(QString::fromStdString(cardSerial), 8, '0'));
  ui->intervalSpinBox->setValue(30);

  //TODO move reading to separate thread


  try{
    if (libada::i()->isTOTPSlotProgrammed(slotNo)) {
      //FIXME use separate thread
      auto p = nm::instance()->get_TOTP_slot_data(slotNo);
      updateSlotConfig(p, ui);
      uint64_t interval = p.slot_counter;
      if (interval < 1) interval = 30;
      ui->intervalSpinBox->setValue(interval);
    }
  }
  catch (DeviceCommunicationException &e){
    emit DeviceDisconnected();
    return;
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
  ui->secretEdit->clear();
  ui->secretEdit->setPlaceholderText("********************************");

  ui->nameEdit->setText(QString::fromStdString(libada::i()->getHOTPSlotName(slotNo)));

  ui->base32RadioButton->setChecked(true);
  std::string cardSerial = libada::i()->get_serial_number();
  ui->muiEdit->setText(QString("%1").arg(QString::fromStdString(cardSerial), 8, '0'));
  ui->ompEdit->setText("NK");
  ui->ttEdit->setText("01");
  ui->counterEdit->setText(QString::number(0));

  try {
    if (libada::i()->isHOTPSlotProgrammed(slotNo)) {
      //FIXME use separate thread
      auto p = nm::instance()->get_HOTP_slot_data(slotNo);
      updateSlotConfig(p, ui);
      ui->counterEdit->setText(QString::number(p.slot_counter));
    }
  }
  catch (DeviceCommunicationException &e){
    emit DeviceDisconnected();
    return;
  }
}

void MainWindow::displayCurrentSlotConfig() {
  ui->slotComboBox->setEnabled(false);
  ui->slotComboBox->repaint();

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
  ui->slotComboBox->setEnabled(true);
  checkTextEdited();
}

void MainWindow::displayCurrentGeneralConfig() {
  auto status = libada::i()->get_status();

  ui->numLockComboBox->setCurrentIndex(status.numlock<2?status.numlock+1:0);
  ui->capsLockComboBox->setCurrentIndex(status.capslock<2?status.capslock+1:0);
  ui->scrollLockComboBox->setCurrentIndex(status.scrolllock<2?status.scrolllock+1:0);

  ui->enableUserPasswordCheckBox->setChecked(status.enable_user_password != 0);
  ui->deleteUserPasswordCheckBox->setChecked(status.delete_user_password != 0);
}

void MainWindow::startConfiguration() {
  bool validPassword = true;
  // Start the config dialog
  if (validPassword) {
    displayCurrentSlotConfig();
    displayCurrentGeneralConfig();
    SetupPasswordSafeConfig();

    ManageWindow::bringToFocus(this);

    QTimer::singleShot(0, this, SLOT(resizeMin()));
  }
  if (libada::i()->isStorageDeviceConnected()) {
    ui->counterEdit->setMaxLength(7);
  }
}

void MainWindow::resizeMin() { resize(minimumSizeHint()); }


void MainWindow::startStickDebug() {
  debug->show();
  debug->raise();
}

#include <QDesktopServices>
#include <QUrl>
void MainWindow::startHelpAction() {
    QString link = "https://www.nitrokey.com/start";
    QDesktopServices::openUrl(QUrl(link));
}

void MainWindow::startAboutDialog() {
  AboutDialog dialog(this);
  dialog.exec();
}


void MainWindow::startStick20ActionChangeUpdatePIN() {
  DialogChangePassword dialog(this, PasswordKind::UPDATE);
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ActionChangeUserPIN() {
  DialogChangePassword dialog(this, PasswordKind::USER);
  connect(&dialog, SIGNAL(UserPinLocked()), &tray, SLOT(regenerateMenu()));
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startStick20ActionChangeAdminPIN() {
  DialogChangePassword dialog(this, PasswordKind::ADMIN);
  dialog.InitData();
  dialog.exec();
}

void MainWindow::startResetUserPassword() {
  DialogChangePassword dialog(this, PasswordKind::RESET_USER);
  dialog.InitData();
  dialog.exec();
}



void MainWindow::storage_check_symlink(){
//TODO make better partition detection method
#ifdef Q_OS_LINUX
    QSettings settings;
    bool should_remind = settings.value("storage/check_symlink", true).toBool();
    if (should_remind && !QFileInfo("/dev/nitrospace").isSymLink()) {
        bool user_wants_reminding = csApplet()->yesOrNoBox(
                                    tr("Warning: Application could not detect any partition on the Encrypted Volume. "
                                       "Please use graphical GParted or terminal fdisk/parted tools for this.")+
                                       " " + tr("Would you like to be reminded again?")
                                    , true);
        if (!user_wants_reminding){
            settings.setValue("storage/check_symlink", false);
        }
    }
#endif
}

//int MainWindow::stick20SendCommand(uint8_t stick20Command, uint8_t *password) {
//  csApplet()->warningBox(tr("There was an error during communicating with device. Please try again."));
//  csApplet()->yesOrNoBox(tr("This command fills the encrypted volumes with random data "
//                                "and will destroy all encrypted volumes!\n"
//                                "It requires more than 1 hour for 32GB. Do you want to continue?"), false);
//  csApplet()->warningBox(tr("Either the password is not correct or the command execution resulted "
//                                "in an error. Please try again."));
//  return (true);
//}

void MainWindow::on_writeButton_clicked() {
  uint8_t slotNo = (uint8_t) ui->slotComboBox->currentIndex();
  const auto isHOTP = slotNo > TOTP_SlotCount;
  slotNo = isHOTP? slotNo - TOTP_SlotCount -1:slotNo;


  if (ui->nameEdit->text().isEmpty()) {
    csApplet()->warningBox(tr("Please enter a slotname."));
    return;
  }

  if (!libada::i()->isDeviceConnected()) {
    csApplet()->warningBox(tr("Nitrokey is not connected!"));
    return;
  }

  OTPSlot otp;
  try{
    if (isHOTP) { // HOTP slot
      generateHOTPConfig(&otp);
    } else {
      generateTOTPConfig(&otp);
    }
  }
  catch(const cppcodec::parse_error &e){
    csApplet()->warningBox(invalid_secret_key_string_details + e.what());
    return;
  }

  if (!this->ui->secretEdit->text().isEmpty() && !validate_raw_secret(otp.secret)) {
    csApplet()->warningBox(invalid_secret_key_string_details + tr("secret is not passing validation."));
    return;
  }

  if(auth_admin.authenticate()){
    try{
      libada::i()->writeToOTPSlot(otp, auth_admin.getTempPassword().constData());
      csApplet()->messageBox(tr("Configuration successfully written."));
      emit OTP_slot_write(slotNo, isHOTP);
    }
    catch (const CommandFailedException&){
      csApplet()->warningBox(tr("Error writing configuration!"));
    }
    catch (const InvalidHexString&){
      csApplet()->warningBox(tr("Provided secret hex string is invalid. Please check input and try again."));
    }
  }

//    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//    QApplication::restoreOverrideCursor();

    generateAllConfigs();
}

bool MainWindow::validate_raw_secret(const char *secret) const {
  if(libada::i()->is_nkpro_07_rtm1() && secret[0] == 0){
      csApplet()->warningBox(tr("Nitrokey Pro v0.7 does not support secrets starting from null byte. Please change the secret."));
    return false;
  }
  //check if the secret consist only from null bytes
  //(this value is reserved - it would be ignored by device)
  for (unsigned int i=0; i < SECRET_LENGTH; i++){
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

  QString secret_input = getOTPSecretCleaned(ui->secretEdit->text());
  ui->secretEdit->setText(secret_input);
  
  auto secret = secret_input.toLatin1().toStdString();
  if (secret.size() != 0) {
    try{
      auto secret_raw = decodeBase32Secret(secret, debug_mode);
      auto secret_hex = QString::fromStdString(cppcodec::hex_upper::encode(secret_raw));
      ui->secretEdit->setText(secret_hex);
      clipboard.copyOTP(secret_hex);
      showNotificationLabel();
    }
    catch (const cppcodec::parse_error &e) {
      ui->secretEdit->setText("");
      csApplet()->warningBox(invalid_secret_key_string_details + e.what());
    }
  }
}

QString MainWindow::getOTPSecretCleaned(QString secret_input) {
  secret_input = secret_input.remove('-').remove(' ').trimmed();
  constexpr auto base32_block_size = 8u;
  int secret_length = std::min(get_supported_secret_length_base32(),
                               roundToNextMultiple(secret_input.length(), base32_block_size));
  secret_input = secret_input.leftJustified(secret_length, '=');
  return secret_input;
}

unsigned int MainWindow::roundToNextMultiple(const int number, const int multipleOf) const {
  return static_cast<unsigned int>(
      number + ((number % multipleOf == 0) ? 0 : multipleOf - number % multipleOf));
}

unsigned int MainWindow::get_supported_secret_length_hex() const {
  auto local_secret_length = SECRET_LENGTH_HEX;
  if (!libada::i()->is_secret320_supported()){
    local_secret_length /= 2;
  }
  return local_secret_length;
}

#include <cppcodec/base32_default_rfc4648.hpp>

void MainWindow::on_base32RadioButton_toggled(bool checked) {
  if (!checked) {
    return;
  }

  auto secret_hex = ui->secretEdit->text().toStdString();
  if (secret_hex.size() != 0) {
    try{
      auto secret_raw = cppcodec::hex_upper::decode(secret_hex);
      auto secret_base32 = QString::fromStdString(base32::encode(secret_raw));
      ui->secretEdit->setText(secret_base32);
      clipboard.copyOTP(secret_base32);
      showNotificationLabel();
    }
    catch (const cppcodec::parse_error &e) {
      ui->secretEdit->setText("");
      csApplet()->warningBox(tr("The secret string you have entered is invalid. Please reenter it.")
                             +" "+tr("Details: ") + e.what());
    }
  }
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
  ui->ompEdit->setEnabled(checked);
  ui->ttEdit->setEnabled(checked);
  ui->muiEdit->setEnabled(checked);
  ui->ompEdit->setText(ui->ompEdit->text().trimmed());
  ui->ttEdit->setText(ui->ttEdit->text().trimmed());
  ui->muiEdit->setText(ui->muiEdit->text().trimmed());
}

void MainWindow::on_enableUserPasswordCheckBox_toggled(bool checked) {
  ui->deleteUserPasswordCheckBox->setEnabled(checked);
  if(checked){
    //TODO run status request in separate thread or cache result
    uint8_t delete_user_password = libada::i()->get_status().delete_user_password;
    ui->deleteUserPasswordCheckBox->setChecked(delete_user_password);
  }
}

void MainWindow::on_writeGeneralConfigButton_clicked() {
  if (!libada::i()->isDeviceConnected()) {
      csApplet()->warningBox(tr("Nitrokey is not connected!"));
  }
    if(!auth_admin.authenticate()){
      csApplet()->warningBox(tr("Wrong PIN. Please try again."));
      return;
    }
  try{
    auto password_byte_array = auth_admin.getTempPassword();
    nm::instance()->write_config(
        ui->numLockComboBox->currentIndex() - 1,
        ui->capsLockComboBox->currentIndex() - 1,
        ui->scrollLockComboBox->currentIndex() - 1,
        ui->enableUserPasswordCheckBox->isChecked(),
        ui->deleteUserPasswordCheckBox->isChecked() &&
        ui->enableUserPasswordCheckBox->isChecked(),
        password_byte_array.constData()
    );
    csApplet()->messageBox(tr("Configuration successfully written."));
  }
  catch (CommandFailedException &e){
    csApplet()->warningBox(tr("Error writing configuration!"));
  }

    generateAllConfigs();
  displayCurrentGeneralConfig();
}

void MainWindow::getHOTPDialog(int slot) {
  try{
    auto OTPcode = getNextCode(0x10 + slot);
    if (OTPcode.empty()) return;
    clipboard.copyOTP(QString::fromStdString(OTPcode));


    if (libada::i()->getHOTPSlotName(slot).empty())
      tray.showTrayMessage(QString(tr("HOTP slot ")).append(QString::number(slot + 1, 10)),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
    else
      tray.showTrayMessage(QString(tr("HOTP slot "))
                          .append(QString::number(slot + 1, 10))
                          .append(" [")
                          .append(QString::fromStdString(libada::i()->getHOTPSlotName(slot)))
                          .append("]"),
                      tr("One-time password has been copied to clipboard."), INFORMATION,
                      TRAY_MSG_TIMEOUT);
  }
  catch(DeviceCommunicationException &e){
    tray.showTrayMessage(communication_error_message);
  }
}

void MainWindow::getTOTPDialog(int slot) {
  try{
    auto OTPcode = getNextCode(0x20 + slot);
      if (OTPcode.empty()) return;
    clipboard.copyOTP(QString::fromStdString(OTPcode));

    if (libada::i()->getTOTPSlotName(slot).empty())
      tray.showTrayMessage(QString(tr("TOTP slot ")).append(QString::number(slot + 1, 10)),
                           tr("One-time password has been copied to clipboard."), INFORMATION,
                           TRAY_MSG_TIMEOUT);
    else
      tray.showTrayMessage(QString(tr("TOTP slot "))
                               .append(QString::number(slot + 1, 10))
                               .append(" [")
                               .append(QString::fromStdString(libada::i()->getTOTPSlotName(slot)))
                               .append("]"),
                           tr("One-time password has been copied to clipboard."), INFORMATION,
                           TRAY_MSG_TIMEOUT);
  }
  catch(DeviceCommunicationException &e){
    tray.showTrayMessage(communication_error_message);
  }

}

void MainWindow::on_eraseButton_clicked() {
  bool answer = csApplet()->yesOrNoBox(tr("WARNING: Are you sure you want to erase the slot?"), false);
  if (!answer) {
    return;
  }

  uint8_t slotNo = ui->slotComboBox->currentIndex();

  const auto isHOTP = slotNo > TOTP_SlotCount;
  if (isHOTP) {
    slotNo -= (TOTP_SlotCount + 1);
  }

    if (!auth_admin.authenticate()){
        csApplet()->messageBox(tr("Command execution failed. Please try again."));
        return;
    };
  auto password_array = auth_admin.getTempPassword();
  if (isHOTP) {
    libada::i()->eraseHOTPSlot(slotNo, password_array.constData());
  } else {
    libada::i()->eraseTOTPSlot(slotNo, password_array.constData());
  }
  emit OTP_slot_write(slotNo, isHOTP);
    csApplet()->messageBox(tr("Slot has been erased successfully."));

//  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//  QApplication::restoreOverrideCursor();
  generateAllConfigs();
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
  clipboard.copyOTP(secretArray);
  showNotificationLabel();
  this->checkTextEdited();
}

void MainWindow::showNotificationLabel() {
  static qint64 lastTimeNotificationShown = 0;
  lastTimeNotificationShown = QDateTime::currentMSecsSinceEpoch();
  constexpr auto delayBeforeHiding = 6000;
  constexpr auto timer_precision_off = 1000;
  ui->labelNotify->show();
  QTimer::singleShot(delayBeforeHiding, [this](){
    if (QDateTime::currentMSecsSinceEpoch() >
        lastTimeNotificationShown + delayBeforeHiding - timer_precision_off){
      ui->labelNotify->hide();
    }
  });
}

unsigned int MainWindow::get_supported_secret_length_base32() const {
  auto local_secret_length = SECRET_LENGTH_BASE32;
  if (!libada::i()->is_secret320_supported()){
    local_secret_length /= 2;
  }
  return local_secret_length;
}

void MainWindow::on_checkBox_toggled(bool checked) {
    ui->secretEdit->setEchoMode(checked ? QLineEdit::PasswordEchoOnEdit : QLineEdit::Normal);
}

void MainWindow::checkTextEdited() {
  if (!ui->checkBox->isEnabled()) {
    ui->checkBox->setEnabled(true);
    ui->checkBox->setChecked(false);
  }
  auto secret_key = ui->secretEdit->text();
  bool valid = secret_key.isEmpty();
  bool correct_length = true;

  if (!valid){
    bool base32 = ui->base32RadioButton->isChecked();
    if (base32) {
      auto secret = getOTPSecretCleaned(secret_key).toStdString();
      if (secret.empty()) return;
      try {
        correct_length = secret.length() <= get_supported_secret_length_base32();
        auto secret_raw = decodeBase32Secret(secret, debug_mode);
        valid = validate_raw_secret(reinterpret_cast<const char *>(secret_raw.data()));
        valid = valid && correct_length;
      }
      catch (...) {}
    } else { // hex
      const auto l = static_cast<unsigned int>(secret_key.length());
      correct_length = l <= get_supported_secret_length_hex();
      valid = l % 2 == 0;
      valid = valid && correct_length;
      if (valid){
        try{
          cppcodec::hex_upper::decode(secret_key.toStdString());
        }
        catch (const cppcodec::parse_error &e){
          if (debug_mode)
            qDebug() << e.what();
          valid = false;
        }
      }
    }
  }

  ui->l_supportedLength->setVisible(!correct_length);
  ui->secretEdit->setStyleSheet(valid ? "background: white;" : "background: #FFDDDD;");
  ui->base32RadioButton->setEnabled(valid);
  ui->hexRadioButton->setEnabled(valid);
  ui->writeButton->setEnabled(valid);
}

void MainWindow::SetupPasswordSafeConfig(void) {
  int i;
  QString Slotname;
  ui->PWS_ComboBoxSelectSlot->clear();
  PWS_set_controls_enabled(PWS_Access);

  try{
    libada::i()->get_status(); //WORKAROUND for crashing Storage v0.45
    PWS_Access = libada::i()->isPasswordSafeUnlocked();
    PWS_set_controls_enabled(PWS_Access);

    // Get active password slots
    if (PWS_Access) {
      // Setup combobox
      ui->PWS_ComboBoxSelectSlot->clear();
      ui->PWS_ComboBoxSelectSlot->addItem(QString(tr("<Select Password Safe slot>")));
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

  }
  catch (LongOperationInProgressException &e){
    long_operation_in_progress = true;
    return;
  }

  ui->PWS_ComboBoxSelectSlot->setEnabled(PWS_Access);
  ui->PWS_ButtonEnable->setVisible(!PWS_Access);

  ui->PWS_EditSlotName->setMaxLength(PWS_SLOTNAME_LENGTH);
  ui->PWS_EditPassword->setMaxLength(PWS_PASSWORD_LENGTH);
  ui->PWS_EditLoginName->setMaxLength(PWS_LOGINNAME_LENGTH);

  ui->PWS_CheckBoxHideSecret->setChecked(TRUE);
  ui->PWS_EditPassword->setEchoMode(QLineEdit::Password);
}

void MainWindow::on_PWS_ButtonClearSlot_clicked() {
  const auto item_number = ui->PWS_ComboBoxSelectSlot->currentIndex();
  const int slot_number = item_number - 1;
  if (slot_number<0){
    return;
  }

  bool answer = csApplet()->yesOrNoBox(tr("WARNING: Are you sure you want to erase the slot?"), false);
  if (!answer){
      return;
  }

  if (!libada::i()->getPWSSlotStatus(slot_number)) // Is slot active?
  {
    csApplet()->messageBox(tr("Slot is erased already."));
    return;
  }

  try{
    libada::i()->erasePWSSlot(slot_number);

    ui->PWS_EditSlotName->setText("");
    ui->PWS_EditPassword->setText("");
    ui->PWS_EditLoginName->setText("");
    ui->PWS_ComboBoxSelectSlot->setItemText(
        item_number, tr("Slot ").append(QString::number(slot_number + 1)));
    csApplet()->messageBox(tr("Slot has been erased successfully."));
    emit PWS_slot_saved(slot_number);
  }
  catch (CommandFailedException &e){
    csApplet()->warningBox(tr("Can't clear slot."));
  }
}

void clear_free_cstr(char *_cstr){
  auto max_string_length = 30ul;
  volatile char* cstr = _cstr;
  for (size_t i = 0; i < max_string_length; ++i) {
    if (cstr[i] == 0) break;
    cstr[i] = ' ';
  }
  free((void *) _cstr);
}

#include "src/core/ThreadWorker.h"
void MainWindow::on_PWS_ComboBoxSelectSlot_currentIndexChanged(int index) {
  auto dummy_slot = index <= 0;

  PWS_set_controls_enabled(!dummy_slot);

  if (dummy_slot) return; //do not update for dummy slot
  index--;

  if (!PWS_Access) {
    return;
  }
  ui->PWS_ComboBoxSelectSlot->setEnabled(false);

  ui->PWS_progressBar->show();
  connect(this, SIGNAL(PWS_progress(int)), ui->PWS_progressBar, SLOT(setValue(int)));

  new ThreadWorker(
    [index, this]() -> Data {
      Data data;
        data["slot_filled"] = libada::i()->getPWSSlotStatus(index);
        emit PWS_progress(100*1/4);
        if (data["slot_filled"].toBool()) {
          data["name"] = QString::fromStdString(libada::i()->getPWSSlotName(index));
          emit PWS_progress(100*2/4);
          //FIXME use secure way
          auto pass_cstr = nm::instance()->get_password_safe_slot_password(index);
          data["pass"] = QString::fromStdString(pass_cstr);
          clear_free_cstr(const_cast<char*>(pass_cstr));
          emit PWS_progress(100*3/4);
          auto login_cstr = nm::instance()->get_password_safe_slot_login(index);
          data["login"] = QString::fromStdString(login_cstr);
          clear_free_cstr(const_cast<char*>(login_cstr));
        }
        emit PWS_progress(100*4/4);
        return data;
    },
    [this](Data data){
      if (data["slot_filled"].toBool()) {
        ui->PWS_EditSlotName->setText(data["name"].toString());
        ui->PWS_EditPassword->setText(data["pass"].toString());
        ui->PWS_EditLoginName->setText(data["login"].toString());
      }
      ui->PWS_ComboBoxSelectSlot->setEnabled(true);
        QTimer::singleShot(2000, [this](){
            ui->PWS_progressBar->hide();
        });

    }, this);

}

void MainWindow::PWS_set_controls_enabled(bool enabled) const {
  ui->PWS_EditSlotName->setText("");
  ui->PWS_EditLoginName->setText("");
  ui->PWS_EditPassword->setText("");
  ui->PWS_EditSlotName->setEnabled(enabled);
  ui->PWS_EditLoginName->setEnabled(enabled);
  ui->PWS_EditPassword->setEnabled(enabled);
  ui->PWS_ButtonSaveSlot->setEnabled(enabled);
  ui->PWS_ButtonClearSlot->setEnabled(enabled);
  ui->PWS_CheckBoxHideSecret->setEnabled(enabled);
  ui->PWS_ButtonCreatePW->setEnabled(enabled);

}

void MainWindow::on_PWS_CheckBoxHideSecret_toggled(bool checked) {
    ui->PWS_EditPassword->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
}

void MainWindow::on_PWS_ButtonSaveSlot_clicked() {
  const auto item_number = ui->PWS_ComboBoxSelectSlot->currentIndex();
  int slot_number = item_number - 1;
  if(slot_number<0) return;

  if(ui->PWS_EditSlotName->text().isEmpty()){
    csApplet()->warningBox(tr("Please enter a slotname."));
    return;
  }
  if(ui->PWS_EditPassword->text().isEmpty()){
    csApplet()->warningBox(tr("Please enter a password."));
    return;
  }

  try{
    nm::instance()->write_password_safe_slot(slot_number,
       ui->PWS_EditSlotName->text().toUtf8().constData(),
       ui->PWS_EditLoginName->text().toUtf8().constData(),
       ui->PWS_EditPassword->text().toUtf8().constData());
    emit PWS_slot_saved(slot_number);
    auto item_name = tr("Slot ")
        .append(QString::number(item_number))
        .append(QString(" [")
                    .append(ui->PWS_EditSlotName->text())
                    .append(QString("]")));
    ui->PWS_ComboBoxSelectSlot->setItemText(
        item_number, item_name);
    csApplet()->messageBox(tr("Slot successfully written."));
  }
  catch (CommandFailedException &e){
    csApplet()->messageBox(tr("Can't save slot. %1").arg(e.last_command_status));
  }
  catch (DeviceCommunicationException &e){
    csApplet()->messageBox(tr("Can't save slot. %1").arg(communication_error_message));
  }
}


void MainWindow::on_PWS_ButtonClose_pressed() { hide(); }

void MainWindow::PWS_Clicked_EnablePWSAccess() {
  do{
    try{
      auto user_password = auth_user.getPassword();
      if(user_password.empty()) return;
      nm::instance()->enable_password_safe(user_password.c_str());
      tray.showTrayMessage(tr("Password safe unlocked"));
      PWS_Access = true;
      emit PWS_unlocked();
      return;
    }
    catch (DeviceCommunicationException &e){
      csApplet()->warningBox(communication_error_message);
    }
    catch (CommandFailedException &e){
      //TODO emit pw safe not available when not available
  //    cryptostick->passwordSafeAvailable = FALSE;
  //    UnlockPasswordSafeAction->setEnabled(false);
  //    csApplet()->warningBox(tr("Password safe is not supported by this device."));
  //    ui->tabWidget->setTabEnabled(3, 0);

      if(e.reason_wrong_password()){
        //show message if wrong password
        csApplet()->warningBox(tr("Wrong user password."));
      } else if (e.reason_AES_not_initialized()){
        //generate keys if not generated
        try{
          csApplet()->warningBox(tr("AES keys not initialized. Please provide Admin PIN."));
          nm::instance()->build_aes_key(auth_admin.getPassword().c_str());
          csApplet()->messageBox(tr("Keys generated. Please unlock Password Safe again."));
        } catch (CommandFailedException &e){
          if (e.reason_wrong_password())
            csApplet()->warningBox(tr("Wrong admin password."));
          else {
            csApplet()->warningBox(tr("Can't unlock password safe."));
          }
        }
      } else {
        //otherwise
        csApplet()->warningBox(tr("Can't unlock password safe."));
      }
    }
  } while (true);
}

void MainWindow::PWS_ExceClickedSlot(int Slot) {
  try {
    auto slot_password = nm::instance()->get_password_safe_slot_password((uint8_t) Slot);
    clipboard.copyPWS(slot_password);
    clear_free_cstr(const_cast<char*>(slot_password));
    QString password_safe_slot_info =
        QString(tr("Password safe [%1]").arg(QString::fromStdString(libada::i()->getPWSSlotName(Slot))));
    QString title = QString("Password has been copied to clipboard");
    tray.showTrayMessage(title, password_safe_slot_info);
  }
  catch(DeviceCommunicationException){
    tray.showTrayMessage(communication_error_message);
  }
}

#include "GUI/Authentication.h"
std::string MainWindow::getNextCode(uint8_t slotNumber) {
    const auto status = nm::instance()->get_status();
    QByteArray tempPassword;

    if(status.enable_user_password){
        if(!auth_user.authenticate()){
          csApplet()->messageBox(tr("User not authenticated"));
          return "";
        }
        tempPassword = auth_user.getTempPassword();
    }
  bool isTOTP = slotNumber >= 0x20;
  auto temp_password_byte_array = tempPassword;
  if (isTOTP){
    //run only once before first TOTP request
    static bool time_synchronized = libada::i()->is_time_synchronized();
    if (!time_synchronized) {
       bool user_wants_time_reset =
           csApplet()->detailedYesOrNoBox(tr("Time is out-of-sync") + " - " + RESET_NITROKEYS_TIME,
                                          WARNING_DEVICES_CLOCK_NOT_DESYNCHRONIZED, false);
      if (user_wants_time_reset){
          if(libada::i()->set_current_time()){
            tray.showTrayMessage(tr("Time reset!"));
            time_synchronized = true;
          }
      }
    }
    return libada::i()->getTOTPCode(slotNumber - 0x20, (const char *) temp_password_byte_array.constData());
  } else {
    return libada::i()->getHOTPCode(slotNumber - 0x10, (const char *) temp_password_byte_array.constData());
  }
  return 0;
}


#define PWS_RANDOM_PASSWORD_CHAR_SPACE                                                             \
  "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"$%&/"                          \
  "()=?[]{}~*+#_'-`,.;:><^|@\\"

void MainWindow::on_PWS_ButtonCreatePW_clicked() {
  //FIXME generate in separate class
  int n;
  const QString PasswordCharSpace = PWS_RANDOM_PASSWORD_CHAR_SPACE;
  QString generated_password(20, 0);

  for (int i = 0; i < PWS_CreatePWSize; i++) {
    n = qrand();
    n = n % PasswordCharSpace.length();
    generated_password[i] = PasswordCharSpace[n];
  }
  ui->PWS_EditPassword->setText(generated_password.toLocal8Bit());
}

void MainWindow::on_PWS_ButtonEnable_clicked() { PWS_Clicked_EnablePWSAccess(); }

void MainWindow::on_counterEdit_editingFinished() {
  bool conversionSuccess = false;
  ui->counterEdit->text().toLatin1().toULongLong(&conversionSuccess);
  if (!libada::i()->isStorageDeviceConnected()) {
    quint64 counterMaxValue = ULLONG_MAX;
    if (!conversionSuccess) {
      ui->counterEdit->setText(QString("%1").arg(0));
        csApplet()->warningBox(tr("Counter must be a value between 0 and %1").arg(counterMaxValue));
    }
  } else { // for nitrokey storage
    if (!conversionSuccess || ui->counterEdit->text().toLatin1().length() > 7) {
      ui->counterEdit->setText(QString("%1").arg(0));
        csApplet()->warningBox(tr("For Nitrokey Storage counter must be a value between 0 and 9999999"));
    }
  }
}

int MainWindow::factoryResetAction() {
  while (true){
    const std::string &password = auth_admin.getPassword();
    if (password.empty())
      return 0;
    try{
      nm::instance()->factory_reset(password.c_str());
    }
    catch (InvalidCRCReceived &e) {
      //FIXME WORKAROUND to remove on later firmwares
      //We are expecting this exception due to bug in Storage stick firmware, v0.45, with CRC set to "0" in response
    }
    catch (CommandFailedException &e){
      if(!e.reason_wrong_password())
        throw;
      csApplet()->messageBox(tr("Wrong Pin. Please try again."));
      continue;
    }
    csApplet()->messageBox(tr("Factory reset was successful."));
    emit FactoryReset();
    return 1;
  }
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
  if (checked && libada::i()->is_nkpro_07_rtm1()) {
    bool answer = csApplet()->yesOrNoBox(tr("To handle this functionality "
                                                    "application will keep your user PIN in memory. "
                                                    "Do you want to continue?"), false);
    ui->enableUserPasswordCheckBox->setChecked(answer);
    //TODO save choice in APP's configuration
  }
}

void MainWindow::startLockDeviceAction(bool ask_for_confirmation) {
  if(libada::i()->isStorageDeviceConnected()){
    PWS_Access = false;
    storage.startLockDeviceAction(ask_for_confirmation);
    return;
  }
  emit ShortOperationBegins(tr("Locking device"));
  PWS_Access = false;
  nm::instance()->lock_device();
  tray.showTrayMessage("Nitrokey App", tr("Device has been locked"), INFORMATION, TRAY_MSG_TIMEOUT);
  emit DeviceLocked();
  emit ShortOperationEnds();
}

void MainWindow::updateProgressBar(int i) {
  ui->progressBar->setValue(i);
  if(i == 100){
    QTimer::singleShot(1000, [&](){
      ui->progressBar->hide();
    });
  }
}

void MainWindow::on_DeviceDisconnected() {
  if (debug_mode)
    qDebug("on_DeviceDisconnected");

  emit ShortOperationEnds();

  QSettings settings;
  if(settings.value("main/connection_message", true).toBool()){
    ui->statusBar->showMessage(tr("Nitrokey disconnected"));
    tray.showTrayMessage(tr("Nitrokey disconnected"));
  }

  if(this->isVisible()){
    this->close();
    csApplet()->messageBox(tr("Closing window due to device disconnection"));
  }
}

#include "src/core/ThreadWorker.h"
#include "hotpslot.h"

void MainWindow::on_DeviceConnected() {
  if (debug_mode)
    qDebug("on_DeviceConnected");

  if (debug_mode)
    emit ShortOperationBegins(tr("Connecting device"));

  //TODO share device state to improve performance
  try{
    libada::i()->get_status();
  }
  catch (LongOperationInProgressException &e){
    long_operation_in_progress = true;
    return;
  }

  QSettings settings;
  if(settings.value("main/connection_message", true).toBool()){

     auto connected_device_model = libada::i()->isStorageDeviceConnected() ?
                                tr("Nitrokey Storage connected") :
                                tr("Nitrokey Pro connected");
    ui->statusBar->showMessage(connected_device_model);
    tray.showTrayMessage(tr("Nitrokey connected"), connected_device_model);
  }

  initialTimeReset();

  new ThreadWorker(
    []() -> Data {
      Data data;
      data["error"] = false;
      try{
        auto storageDeviceConnected = libada::i()->isStorageDeviceConnected();
        data["storage_connected"] = storageDeviceConnected;
        if (storageDeviceConnected){
          auto s = nm::instance()->get_status_storage();
          data["initiated"] = !s.StickKeysNotInitiated;
          data["initiated_ask"] = !false; //FIXME select proper variable s.NewSDCardFound_u8
          data["erased"] = !s.NewSDCardFound_u8;
          data["erased_ask"] = !s.SDFillWithRandomChars_u8;
          data["old_firmware"] = s.versionInfo.major == 0 && s.versionInfo.minor <= 49;
        }
        data["PWS_Access"] = libada::i()->isPasswordSafeUnlocked();
      }
      catch(DeviceCommunicationException &e){
        data["error"] = true;
      }
      return data;
    },
    [this](Data data) {
      emit ShortOperationEnds();
      PWS_Access = data["PWS_Access"].toBool();
      if(data["error"].toBool()) return;
      if(!data["storage_connected"].toBool()) return;

      if (!data["initiated"].toBool()) {
        if (data["initiated_ask"].toBool())
          csApplet()->warningBox(tr("Warning: Encrypted volume is not secure,\nSelect \"Initialize "
                                        "device\" option from context menu.") + " " + tray_location_msg);
      }
      if (data["initiated"].toBool() && !data["erased"].toBool()) {
        if (data["erased_ask"].toBool())
          csApplet()->warningBox(tr("Warning: Encrypted volume is not secure,\nSelect \"Initialize "
                                        "storage with random data\"") + ". " + tray_location_msg);
      }

#if defined(Q_OS_MAC) || defined(Q_OS_DARWIN)
      if(data["old_firmware"].toBool()){
        csApplet()->warningBox(tr(
                                  "WARNING: This Storage firmware version is old. Application may be unresponsive and unlocking encrypted volume may not work. Please update the firmware to the latest version."
                                   " "
                                   "Guide should be available at: <br/><a href='https://www.nitrokey.com/en/doc/firmware-update-storage'>www.nitrokey.com/en/doc/firmware-update-storage</a>."
                                  ));
      }
#endif
      }, this);  
}

void MainWindow::on_KeepDeviceOnline() {

  if (!check_connection_mutex.tryLock(100)){
    if (debug_mode)
      qDebug("on_KeepDeviceOnline skip");
      return;
  }
  ScopedGuard mutexGuard([this](){
      check_connection_mutex.unlock();
  });

  try{
    nm::instance()->get_status();
    //if long operation in progress jump to catch,
    // clear the flag otherwise
    if (long_operation_in_progress) {
      long_operation_in_progress = false;
      keepDeviceOnlineTimer->setInterval(30*1000);
      emit OperationInProgress(100);
      startLockDeviceAction(false);
    }
  }
  catch (DeviceCommunicationException &e){
    if(connectionState != ConnectionState::disconnected){
      emit DeviceDisconnected();
    }
  }
  catch (LongOperationInProgressException &e){
    if(!long_operation_in_progress){
      long_operation_in_progress = true;
      keepDeviceOnlineTimer->setInterval(10*1000);
    }
    emit OperationInProgress(e.progress_bar_value);
  }
}

void MainWindow::show_progress_window() {
  progress_window->show();
  progress_window->setFocus();
  progress_window->raise();
  QApplication::setActiveWindow(progress_window.get());
}

void MainWindow::set_commands_delay(int delay_in_ms) {
  nm::instance()->set_default_commands_delay(delay_in_ms);
}

void MainWindow::enable_admin_commands() {
  tray.setAdmin_mode(true);
}

void MainWindow::set_debug_file(QString log_file_name) {
  nm::instance()->set_log_function( [log_file_name, debug_mode=this->debug_mode](std::string data){
      static std::shared_ptr<QFile> log_file;
      if(!log_file){
        log_file = std::make_shared<QFile>(log_file_name);
        if (!log_file->open(QIODevice::WriteOnly | QIODevice::Text)){
          if (debug_mode)
            qDebug() << "Could not open " << log_file_name;
          log_file = nullptr;
        }
      }
      if(log_file) {
        log_file->write(data.c_str());
        log_file->flush();
      }
    }
  );
  LOGD(QSysInfo::prettyProductName().toStdString());
}

void MainWindow::set_debug_window() {
  tray.setDebug_mode(true);
  nm::instance()->set_log_function( [this](std::string data) {
      emit DebugData(QString::fromStdString(data));
  });
  LOGD(QSysInfo::prettyProductName().toStdString());
}

void MainWindow::set_debug_mode() {
  tray.setDebug_mode(true);
  debug_mode = true;
}

void MainWindow::set_debug_level(int debug_level) {
  nm::instance()->set_loglevel(debug_level);
}

void MainWindow::on_btn_writeSettings_clicked()
{
    QSettings settings;

    // see if restart is required
    bool restart_required = false;
    if (settings.value("main/language").toString() != ui->combo_languages->currentData().toString()){
        restart_required = true;
    }

    // save the settings
    settings.setValue("main/first_run", ui->cb_first_run_message->isChecked());
    settings.setValue("main/language", ui->combo_languages->currentData());
    settings.setValue("debug/file", ui->edit_debug_file_path->text());
    settings.setValue("debug/level", ui->spin_debug_verbosity->text().toInt());
    settings.setValue("debug/enabled", ui->cb_debug_enabled->isChecked());
    settings.setValue("clipboard/PWS_time", ui->spin_PWS_time->value());
    settings.setValue("clipboard/OTP_time", ui->spin_OTP_time->value());
    settings.setValue("main/connection_message", ui->cb_device_connection_message->isChecked());
    settings.setValue("storage/check_symlink", ui->cb_check_symlink->isChecked());

    // inform user and quit if asked
    if (!restart_required){
        csApplet()->messageBox(tr("Configuration successfully written."));
    } else {
        auto user_wants_quit = csApplet()->yesOrNoBox(
                tr("Configuration successfully written.") + " "+
                tr("Please run the application again to apply new settings.") + " "+
                tr("Would you like to quit now?"), true);
        if (user_wants_quit)
        {
            QApplication::exit();
        }
    }
    load_settings();
}

void MainWindow::on_btn_select_debug_file_path_clicked()
{
    auto filename = QFileDialog::getSaveFileName(this, tr("Debug file location (will be overwritten)"));
    ui->edit_debug_file_path->setText(filename);
}
