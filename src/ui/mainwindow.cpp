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

using nm = nitrokey::NitrokeyManager;
static const QString communication_error_message = QApplication::tr("Communication error. Please reinsert the device.");


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
       clipboard(this), auth_admin(this, Authentication::Type::ADMIN),
      auth_user(this, Authentication::Type::USER), storage(this, &auth_admin, &auth_user),
      tray(this, false, true, &storage),
      HOTP_SlotCount(HOTP_SLOT_COUNT), TOTP_SlotCount(TOTP_SLOT_COUNT)
{

  progress_window = std::make_shared<Stick20ResponseDialog>();

  //TODO make connections in objects instead of accumulating them here
  connect(&storage, SIGNAL(storageStatusChanged()), &tray, SLOT(regenerateMenu()));
  connect(&storage, SIGNAL(FactoryReset()), &tray, SLOT(regenerateMenu()));
  connect(&storage, SIGNAL(FactoryReset()), libada::i().get(), SLOT(on_FactoryReset()));
  connect(&storage, SIGNAL(longOperationStarted()), this, SLOT(on_KeepDeviceOnline()));
  connect(this, SIGNAL(FactoryReset()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(FactoryReset()), libada::i().get(), SLOT(on_FactoryReset()));

  connect(this, SIGNAL(PWS_unlocked()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(PWS_unlocked()), this, SLOT(SetupPasswordSafeConfig()));
  connect(this, SIGNAL(PWS_slot_saved(int)), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(OTP_slot_write(int, bool)), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(DeviceLocked()), &tray, SLOT(regenerateMenu()));
  connect(this, SIGNAL(DeviceLocked()), &storage, SLOT(on_StorageStatusChanged()));
  connect(this, SIGNAL(DeviceConnected()), &storage, SLOT(on_StorageStatusChanged()));
  connect(&tray, SIGNAL(progress(int)), this, SLOT(updateProgressBar(int)));
  connect(this, SIGNAL(OperationInProgress(int)), &tray, SLOT(updateOperationInProgressBar(int)));
  connect(this, SIGNAL(OperationInProgress(int)), progress_window.get(), SLOT(updateOperationInProgressBar(int)));
  connect(this, SIGNAL(OTP_slot_write(int, bool)), libada::i().get(), SLOT(on_OTP_save(int, bool)));
  connect(this, SIGNAL(PWS_slot_saved(int)), libada::i().get(), SLOT(on_PWS_save(int)));
  connect(this, SIGNAL(DeviceDisconnected()), this, SLOT(on_DeviceDisconnected()));
  connect(this, SIGNAL(DeviceDisconnected()), libada::i().get(), SLOT(on_DeviceDisconnect()));
  connect(this, SIGNAL(DeviceConnected()), this, SLOT(on_DeviceConnected()));

  ui->setupUi(this);
  ui->tabWidget->setCurrentIndex(0); // Set first tab active
  ui->PWS_ButtonCreatePW->setText(QString(tr("Generate random password ")));
  ui->PWS_progressBar->hide();
  ui->statusBar->showMessage(tr("Nitrokey disconnected"));

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
  timer->start(2000);
  QTimer::singleShot(500, this, SLOT(checkConnection()));

  keepDeviceOnlineTimer = new QTimer(this);
  connect(keepDeviceOnlineTimer, SIGNAL(timeout()), this, SLOT(on_KeepDeviceOnline()));
  keepDeviceOnlineTimer->start(30*1000);

  connect(ui->secretEdit, SIGNAL(textEdited(QString)), this, SLOT(checkTextEdited()));

  ui->deleteUserPasswordCheckBox->setEnabled(false);
  ui->deleteUserPasswordCheckBox->setChecked(false);

  //TODO
//    translateDeviceStatusToUserMessage(cryptostick->getStatus());

  ui->PWS_EditPassword->setValidator(
      new utf8FieldLengthValidator(PWS_PASSWORD_LENGTH, ui->PWS_EditPassword));
  ui->PWS_EditLoginName->setValidator(
      new utf8FieldLengthValidator(PWS_LOGINNAME_LENGTH, ui->PWS_EditLoginName));
  ui->PWS_EditSlotName->setValidator(
      new utf8FieldLengthValidator(PWS_SLOTNAME_LENGTH, ui->PWS_EditSlotName));

  this->adjustSize();
  this->move(QApplication::desktop()->screen()->rect().center() - this->rect().center());

}

void MainWindow::translateDeviceStatusToUserMessage(const int getStatus){
    switch (getStatus) {
        case 1:
            //regained connection
            tray.showTrayMessage(
                tr("Regained connection to the device.")
            );
        break;
        case -10:
            // problems with communication, received CRC other than expected, try to reinitialize
          tray.showTrayMessage(
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


enum class ConnectionState{
  disconnected, connected, long_operation
};

void MainWindow::checkConnection() {
  using cs = ConnectionState;
  QMutexLocker locker(&check_connection_mutex);

  static ConnectionState state = cs::disconnected;
  bool deviceConnected = libada::i()->isDeviceConnected();

  if (deviceConnected){
    if(state == cs::disconnected){
      state = cs::connected;
      nitrokey::NitrokeyManager::instance()->connect();

      if(libada::i()->isStorageDeviceConnected()){
        try {
          libada::i()->isPasswordSafeUnlocked();
          long_operation_in_progress = false;
        }
        catch (LongOperationInProgressException &e){
//          state = cs::long_operation;
          long_operation_in_progress = true;
          emit OperationInProgress(e.progress_bar_value);
          return;
        }

      }

      //on connection
      emit DeviceConnected();
    }
  } else { //device not connected
    if(state == cs::connected){
      state = cs::disconnected;
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
    bool answer = csApplet()->detailedYesOrNoBox(tr("Time is out-of-sync"),
      tr("WARNING!\n\nThe time of your computer and Nitrokey are out of "
                 "sync. Your computer may be configured with a wrong time or "
                 "your Nitrokey may have been attacked. If an attacker or "
                 "malware could have used your Nitrokey you should reset the "
                 "secrets of your configured One Time Passwords. If your "
                 "computer's time is wrong, please configure it correctly and "
                 "reset the time of your Nitrokey.\n\nReset Nitrokey's time?"),
      false);
    if (answer) {
      auto res = libada::i()->set_current_time();
      if (res) {
        csApplet()->messageBox(tr("Time reset!"));
      }
    }
  }
}

MainWindow::~MainWindow() {
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
//    if (libada::i()->isStorageDeviceConnected() == false) {
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
//  else { // nitrokey storage version
//      QByteArray counterFromGUI = QByteArray(ui->counterEdit->text().toLatin1());
//      int digitsInCounter = counterFromGUI.length();
//      if (0 < digitsInCounter && digitsInCounter < 8) {
//        memcpy(slot->counter, counterFromGUI.constData(), std::min(counterFromGUI.length(), 7));
//        // 8th char has to be '\0' since in firmware atoi is used directly on buffer
//        slot->counter[7] = 0;
//      } else {
//          csApplet()->warningBox(tr("Counter value not copied - Nitrokey Storage handles HOTP counter "
//                                            "values up to 7 digits. Setting counter value to 0. Please retry."));
//      }
//    }

//  }
}

#include <cppcodec/base32_default_rfc4648.hpp>
#include <cppcodec/hex_upper.hpp>
void MainWindow::generateOTPConfig(OTPSlot *slot) const {
  using hex = cppcodec::hex_upper;
  auto secretFromGUI = this->ui->secretEdit->text().toStdString();

  auto secret_raw = base32::decode(secretFromGUI);
  auto secret_hex = hex::encode(secret_raw);

  qDebug() << secret_hex.c_str();
  size_t toCopy = std::min(sizeof(slot->secret), (const size_t &) secret_hex.length());
  if (!libada::i()->is_secret320_supported() && toCopy > 40){
    toCopy = 40;
  }
  memset(slot->secret, 0, sizeof(slot->secret));
  std::copy(secret_hex.begin(), secret_hex.begin()+toCopy, slot->secret);

  //TODO to rewrite
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
  ui->ompEdit->setText(QString((char *) p.slot_token_fields.omp));
  ui->ttEdit->setText(QString((char *) p.slot_token_fields.tt));
  ui->muiEdit->setText(QString((char *) p.slot_token_fields.mui));

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

  //TODO readout TOTP slot data
  //TODO implement reading slot data in libnitrokey
  //TODO move reading to separate thread

  try{
    if (libada::i()->isTOTPSlotProgrammed(slotNo)) {
      //FIXME use separate thread
      auto p = nm::instance()->get_TOTP_slot_data(slotNo);
      updateSlotConfig(p, ui);
      int interval = p.slot_counter;
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
  //TODO authenticate admin plain
//  PinDialog dialog(tr("Enter card admin PIN"), tr("Admin PIN:"), cryptostick, PinDialog::PLAIN,
//  csApplet()->warningBox(tr("Wrong PIN. Please try again."));

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

//void MainWindow::refreshStick20StatusData() {
//  if (TRUE == libada::i()->isStorageDeviceConnected()) {
//    // Get actual data from stick 20
//    cryptostick->stick20GetStatusData();
//    Stick20ResponseTask ResponseTask(this, cryptostick, trayIcon);
//    ResponseTask.NoStopWhenStatusOK();
//    ResponseTask.GetResponse();
//    UpdateDynamicMenuEntrys(); // Use new data to update menu
//  }
//}

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
    if (!QFileInfo("/dev/nitrospace").isSymLink()) {
        csApplet()->warningBox(tr("Warning: The encrypted Volume is not formatted.\n\"Use GParted "
                                          "or fdisk for this.\""));
    }
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


  if (ui->nameEdit->text().isEmpty()) {
    csApplet()->warningBox(tr("Please enter a slotname."));
//          csApplet()->warningBox(tr("The name of the slot must not be empty."));
    return;
  }

  if (!libada::i()->isDeviceConnected()) {
    csApplet()->warningBox(tr("Nitrokey is not connected!"));
    return;
  }
    ui->base32RadioButton->toggle();

    OTPSlot otp;
    if (isHOTP) { // HOTP slot
      generateHOTPConfig(&otp);
    } else {
        generateTOTPConfig(&otp);
    }
    if (!validate_secret(otp.secret)) {
      return;
    }
  qDebug() << otp.secret;
    if(auth_admin.authenticate()){
      try{
        libada::i()->writeToOTPSlot(otp, auth_admin.getTempPassword());
        csApplet()->messageBox(tr("Configuration successfully written."));
        emit OTP_slot_write(slotNo, isHOTP);
      }
      catch (CommandFailedException &e){
        csApplet()->warningBox(tr("Error writing configuration!"));
      }
    }

//    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//    QApplication::restoreOverrideCursor();

    generateAllConfigs();
}

bool MainWindow::validate_secret(const char *secret) const {
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


  auto secret = ui->secretEdit->text().toLatin1().toStdString();
  if (secret.size() != 0) {
    auto secret_raw = base32::decode(secret);
    auto secret_hex = QString::fromStdString(cppcodec::hex_upper::encode(secret_raw));
    ui->secretEdit->setText(secret_hex);
    clipboard.copyToClipboard(secret_hex);
  }
}

int MainWindow::get_supported_secret_length_hex() const {
  //TODO move to libada or libnitrokey
  auto local_secret_length = SECRET_LENGTH_HEX;
  if (!libada::i()->is_secret320_supported()){
    local_secret_length /= 2;
  }
  return local_secret_length;
}

void MainWindow::on_base32RadioButton_toggled(bool checked) {
  //TODO move conversion logic to separate class
  if (!checked) {
    return;
  }

  
  auto secret_hex = ui->secretEdit->text().toStdString();
  if (secret_hex.size() != 0) {
    auto secret_raw = cppcodec::hex_upper::decode(secret_hex);
    auto secret_base32 = QString::fromStdString(base32::encode(secret_raw));

    ui->secretEdit->setText(secret_base32);
    clipboard.copyToClipboard(secret_base32);
  }
  ui->secretEdit->setMaxLength(get_supported_secret_length_base32());
}

void MainWindow::on_setToZeroButton_clicked() { ui->counterEdit->setText("0"); }

void MainWindow::on_setToRandomButton_clicked() {
  quint64 counter;
  counter = qrand();
//  TODO check counter digits limit on storage with libnitrokey
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
      csApplet()->warningBox(tr("Nitrokey not connected!"));
  }
    if(!auth_admin.authenticate()){
      csApplet()->warningBox(tr("Wrong PIN. Please try again."));
      return;
    }
  try{
    auto password_byte_array = auth_admin.getTempPassword().toLatin1();
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
    qDebug() << "should throw?";
  }

//      translateDeviceStatusToUserMessage(cryptostick->getStatus()); //TODO
    generateAllConfigs();
  displayCurrentGeneralConfig();
}

void MainWindow::getHOTPDialog(int slot) {
  try{
    auto OTPcode = getNextCode(0x10 + slot);
    clipboard.copyToClipboard(QString::number(OTPcode));

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
    clipboard.copyToClipboard(QString::number(OTPcode));

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
    int res;
  auto password_array = auth_admin.getTempPassword().toLatin1();
  if (isHOTP) {
    res = libada::i()->eraseHOTPSlot(slotNo, password_array.constData());
  } else {
    res = libada::i()->eraseTOTPSlot(slotNo, password_array.constData());
  }
  emit OTP_slot_write(slotNo, isHOTP);
    csApplet()->messageBox(tr("Slot has been erased successfully."));

    //TODO remove values from OTP name cache
    //TODO regenerate menu (change name for given slot)
    //TODO regenerate combo box (change name for given slot)

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
  clipboard.copyToClipboard(secretArray);
}

int MainWindow::get_supported_secret_length_base32() const {
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
  ui->secretEdit->setText(ui->secretEdit->text().remove(" ").remove("\t"));
  if (!ui->checkBox->isEnabled()) {
    ui->checkBox->setEnabled(true);
    ui->checkBox->setChecked(false);
  }
}

void MainWindow::SetupPasswordSafeConfig(void) {
  int i;
  QString Slotname;
  ui->PWS_ComboBoxSelectSlot->clear();

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

  PWS_Access ? ui->PWS_ButtonEnable->hide() : ui->PWS_ButtonEnable->show();
  ui->PWS_ButtonSaveSlot->setEnabled(PWS_Access);
  ui->PWS_ButtonClearSlot->setEnabled(PWS_Access);
  ui->PWS_ComboBoxSelectSlot->setEnabled(PWS_Access);
  ui->PWS_EditSlotName->setEnabled(PWS_Access);
  ui->PWS_EditLoginName->setEnabled(PWS_Access);
  ui->PWS_EditPassword->setEnabled(PWS_Access);
  ui->PWS_CheckBoxHideSecret->setEnabled(PWS_Access);
  ui->PWS_ButtonCreatePW->setEnabled(PWS_Access);

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
        item_number, QString("Slot ").append(QString::number(slot_number + 1)));
    csApplet()->messageBox(tr("Slot has been erased successfully."));
    emit PWS_slot_saved(slot_number);
  }
  catch (CommandFailedException &e){
    csApplet()->warningBox(tr("Can't clear slot."));
  }
}

#include "src/core/ThreadWorker.h"
void MainWindow::on_PWS_ComboBoxSelectSlot_currentIndexChanged(int index) {
  ui->PWS_EditSlotName->setText("");
  ui->PWS_EditPassword->setText("");
  ui->PWS_EditLoginName->setText("");

  if (index <= 0) return; //do not update for dummy slot
  index--;

  if (!PWS_Access) {
    return;
  }
  ui->PWS_ComboBoxSelectSlot->setEnabled(false);

  ui->PWS_progressBar->show();
  connect(this, SIGNAL(PWS_progress(int)), ui->PWS_progressBar, SLOT(setValue(int)));

  ThreadWorker *tw = new ThreadWorker(
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
          free((void *) pass_cstr);
          emit PWS_progress(100*3/4);
          auto login_cstr = nm::instance()->get_password_safe_slot_login(index);
          data["login"] = QString::fromStdString(login_cstr);
          free((void *) login_cstr);
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

void MainWindow::on_PWS_CheckBoxHideSecret_toggled(bool checked) {
    ui->PWS_EditPassword->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
}

void MainWindow::on_PWS_ButtonSaveSlot_clicked() {

  const auto item_number = ui->PWS_ComboBoxSelectSlot->currentIndex();
  int slot_number = item_number - 1;
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
    auto item_name = QString(tr("Slot "))
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


void MainWindow::on_PWS_ButtonClose_pressed() { hide(); }

void MainWindow::PWS_Clicked_EnablePWSAccess() {
  do{
    try{
      auto user_password = auth_user.getPassword();
      if(user_password.empty()) return;
      nm::instance()->enable_password_safe(user_password.c_str());
      csApplet()->messageBox(tr("Password safe unlocked"));
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
      } else if (e.last_command_status == 0xa){ // FIXME move status code to exception class
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
    clipboard.copyToClipboard(slot_password);
    free((void *) slot_password);
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
int MainWindow::getNextCode(uint8_t slotNumber) {
    const auto status = nm::instance()->get_status();
    QString tempPassword;

    if(status.enable_user_password){
        if(!auth_user.authenticate()){
          csApplet()->messageBox(tr("User not authenticated"));
          return 0;
        }
        tempPassword = auth_user.getTempPassword();
    }
  bool isTOTP = slotNumber >= 0x20;
  auto temp_password_byte_array = tempPassword.toLatin1();
  if (isTOTP){
    //run only once before first TOTP request
    static bool time_synchronized = libada::i()->is_time_synchronized();
    if (!time_synchronized) {
       bool user_wants_time_reset = csApplet()->detailedYesOrNoBox(tr("Time is out-of-sync")
                                                                  + " - " + tr("Reset Nitrokey's time?"),
                  tr("WARNING!\n\nThe time of your computer and Nitrokey are out of "
                             "sync.\nYour computer may be configured with a wrong time "
                             "or\nyour Nitrokey may have been attacked. If an attacker "
                             "or\nmalware could have used your Nitrokey you should reset the "
                             "secrets of your configured One Time Passwords. If your "
                             "computer's time is wrong, please configure it correctly and "
                             "reset the time of your Nitrokey.\n\nReset Nitrokey's time?"),
                  false);
      if (user_wants_time_reset){
          if(libada::i()->set_current_time()){
            csApplet()->messageBox(tr("Time reset!"));
            time_synchronized = true;
          }
      }
    }
    return libada::i()->getTOTPCode(slotNumber - 0x20, temp_password_byte_array.constData());
  } else {
    return libada::i()->getHOTPCode(slotNumber - 0x10, temp_password_byte_array.constData());
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
      csApplet()->messageBox("Factory reset was successful.");
      emit FactoryReset();
      return 1;
    }
    catch (CommandFailedException &e){
      if(!e.reason_wrong_password())
        throw;
      csApplet()->messageBox("Wrong Pin. Please try again.");
    }
  }
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
  }
}

void MainWindow::startLockDeviceAction() {
  nm::instance()->lock_device();
  PWS_Access = false;
  tray.showTrayMessage("Nitrokey App", tr("Device has been locked"), INFORMATION, TRAY_MSG_TIMEOUT);
  emit DeviceLocked();
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
  ui->statusBar->showMessage(tr("Nitrokey disconnected"));
  tray.showTrayMessage(tr("Nitrokey disconnected"));
  tray.regenerateMenu();

  if(this->isVisible()){
    this->close();
    csApplet()->messageBox(tr("Closing window due to device disconnection"));
  }
}

#include "src/core/ThreadWorker.h"
void MainWindow::on_DeviceConnected() {
  //TODO share device state to improve performance
  try{
    PWS_Access = libada::i()->isPasswordSafeUnlocked();
  }
  catch (LongOperationInProgressException &e){
    long_operation_in_progress = true;
    return;
  }

  auto connected_device_model = libada::i()->isStorageDeviceConnected() ?
                                tr("Nitrokey Storage connected") :
                                tr("Nitrokey Pro connected");
  ui->statusBar->showMessage(connected_device_model);
  tray.showTrayMessage(tr("Nitrokey connected"), connected_device_model);
  tray.regenerateMenu();

  initialTimeReset();

//TODO show warnings for storage (test)
ThreadWorker *tw = new ThreadWorker(
    []() -> Data {
      Data data;
      auto storageDeviceConnected = libada::i()->isStorageDeviceConnected();
      data["storage_connected"] = storageDeviceConnected;
      if (storageDeviceConnected){
        auto s = nm::instance()->get_status_storage();
        data["initiated"] = !s.StickKeysNotInitiated;
        data["initiated_ask"] = !false; //FIXME select proper variable s.NewSDCardFound_u8
        data["erased"] = !s.NewSDCardFound_u8;
        data["erased_ask"] = !s.SDFillWithRandomChars_u8; //FIXME s.NewSDCardFound_u8
      }
      return data;
    },
    [this](Data data) {
      if(!data["storage_connected"].toBool()) return;

      if (!data["initiated"].toBool()) {
        if (data["initiated_ask"].toBool())
          csApplet()->warningBox(tr("Warning: Encrypted volume is not secure,\nSelect \"Initialize "
                                        "device\" option from context menu."));
      }
      if (data["initiated"].toBool() && !data["erased"].toBool()) {
        if (data["erased_ask"].toBool())
          csApplet()->warningBox(tr("Warning: Encrypted volume is not secure,\nSelect \"Initialize "
                                        "storage with random data\""));
      }
      }, this);

}

void MainWindow::on_KeepDeviceOnline() {
  qDebug() << "Keeping device online";
  try{
    nm::instance()->get_status();
    //if long operation in progress jump to catch,
    // clear the flag otherwise
    if (long_operation_in_progress) {
      long_operation_in_progress = false;
      keepDeviceOnlineTimer->setInterval(30*1000);
      emit OperationInProgress(100);
      startLockDeviceAction();
    }
  }
  catch (DeviceCommunicationException &e){
    emit DeviceDisconnected();
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
