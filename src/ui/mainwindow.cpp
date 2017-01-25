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
#include "libada.h"

#include <QDateTime>
#include <QDialog>
#include <QMenu>
#include <QThread>
#include <QTimer>
#include <QtWidgets>

#include "src/core/SecureString.h"


#include <QString>
#include <libnitrokey/include/NitrokeyManager.h>
#include <libnitrokey/include/stick10_commands.h>

using nm = nitrokey::NitrokeyManager;


/*******************************************************************************
 Local defines
*******************************************************************************/


void MainWindow::InitState() {
//  Stick20ScSdCardOnline = FALSE;
//  CryptedVolumeActive = FALSE;
//  HiddenVolumeActive = FALSE;
//  NormalVolumeRWActive = FALSE;
//  HiddenVolumeAccessable = FALSE;
//  StickNotInitated = FALSE;
//  SdCardNotErased = FALSE;
//  LockHardware = FALSE;
//  PasswordSafeEnabled = FALSE;
//
//  SdCardNotErased_DontAsk = FALSE;
//  StickNotInitated_DontAsk = FALSE;

  PWS_Access = FALSE;
  PWS_CreatePWSize = 12;
}

//MainWindow::MainWindow(StartUpParameter_tst *StartupInfo_st, QWidget *parent)
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
       clipboard(this), auth_admin(this, Authentication::Type::ADMIN),
      auth_user(this, Authentication::Type::USER), storage(this, &auth_admin, &auth_user),
      tray(this, true, true, &storage),
      HOTP_SlotCount(HOTP_SLOT_COUNT), TOTP_SlotCount(TOTP_SLOT_COUNT)
{
    PWS_Access = FALSE;
  nitrokey::NitrokeyManager::instance()->connect();

  connect(&storage, SIGNAL(storageStatusChanged()), &tray, SLOT(regenerateMenu()));
  connect(&tray, SIGNAL(progress(int)), this, SLOT(updateProgressBar(int)));

  ui->setupUi(this);
  ui->tabWidget->setCurrentIndex(0); // Set first tab active
  ui->PWS_ButtonCreatePW->setText(QString(tr("Generate random password ")));
  ui->statusBar->showMessage(tr("Nitrokey disconnected"));

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(checkConnection()));
  timer->start(2000);


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



void MainWindow::checkConnection() {
  QMutexLocker locker(&check_connection_mutex);

  static int DeviceOffline = TRUE;
  int ret = 0;
//  currentTime = QDateTime::currentDateTime().toTime_t();

  int result = libada::i()->isDeviceConnected()? 0 : -1;
  if (DeviceOffline == TRUE)
    result = 1;

  if (result == 0) { //connected
    if (!libada::i()->isStorageDeviceConnected()) {
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
      tray.regenerateMenu();
      DeviceOffline = TRUE;
      tray.showTrayMessage(tr("Nitrokey disconnected"), "", INFORMATION, TRAY_MSG_TIMEOUT);
    }

  } else if (result == 1) { // recreate the settings and menus
    DeviceOffline = FALSE;
    if (!libada::i()->isStorageDeviceConnected()) {
      ui->statusBar->showMessage(tr("Nitrokey connected"));
      tray.showTrayMessage(tr("Nitrokey connected"), "Nitrokey Pro", INFORMATION, TRAY_MSG_TIMEOUT);
//        translateDeviceStatusToUserMessage(cryptostick->getStatus()); //TODO
    } else {
      ui->statusBar->showMessage(tr("Nitrokey Storage connected"));
      tray.showTrayMessage(tr("Nitrokey connected"), "Nitrokey Storage", INFORMATION, TRAY_MSG_TIMEOUT);
    }
    tray.regenerateMenu();
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

  //TODO use separate base32 encoding class
  base32_clean(encoded, sizeof(data), data);
  base32_decode(data, decoded, sizeof(decoded));

  secretFromGUI = QByteArray((char *)decoded, SECRET_LENGTH).toHex(); //FIXME check
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
  static std::string cardSerial = nm::instance()->get_serial_number();
  ui->muiEdit->setText(QString("%1").arg(QString::fromStdString(cardSerial), 8, '0'));
  ui->intervalSpinBox->setValue(30);

  //TODO readout TOTP slot data
  //TODO implement reading slot data in libnitrokey
  //TODO move reading to separate thread

  if (libada::i()->isTOTPSlotProgrammed(slotNo)) {
    auto p = nm::instance()->get_TOTP_slot_data(slotNo);
    updateSlotConfig(p, ui);
    int interval = p.slot_counter;
    if (interval < 1) interval = 30;
    ui->intervalSpinBox->setValue(interval);
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
  static std::string cardSerial = nm::instance()->get_serial_number();
  ui->muiEdit->setText(QString("%1").arg(QString::fromStdString(cardSerial), 8, '0'));
  ui->ompEdit->setText("NK");
  ui->ttEdit->setText("01");
  ui->counterEdit->setText(QString::number(0));

  if (libada::i()->isHOTPSlotProgrammed(slotNo)) {
    //FIXME use separate thread
    auto p = nm::instance()->get_HOTP_slot_data(slotNo);
    updateSlotConfig(p, ui);
    ui->counterEdit->setText(QString::number(p.slot_counter));
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
}

void MainWindow::displayCurrentGeneralConfig() {
  auto status = nm::instance()->get_status();

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
    if(auth_admin.authenticate()){
      try{
        libada::i()->writeToOTPSlot(otp, auth_admin.getTempPassword());
        csApplet()->messageBox(tr("Configuration successfully written."));
      }
      catch (CommandFailedException &e){
        csApplet()->warningBox(tr("Error writing configuration!"));
      }
    }

//    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
//    QApplication::restoreOverrideCursor();

    generateAllConfigs();
    displayCurrentSlotConfig();
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
  //TODO move conversion logic to separate class
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

    auto secret_str = QString(secret);
    ui->secretEdit->setText(secret_str);
    clipboard.copyToClipboard(secret_str);
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

  QByteArray secret;
  uint8_t encoded[SECRET_LENGTH_BASE32+1] = {}; //+1 for \0
  uint8_t decoded[SECRET_LENGTH] = {};

  secret = QByteArray::fromHex(ui->secretEdit->text().toLatin1());
  if (secret.size() != 0) {
    const size_t decoded_size = std::min(sizeof(decoded), (size_t) secret.length());
    memcpy(decoded, secret.constData(), decoded_size);
    base32_encode(decoded, decoded_size, encoded, sizeof(encoded));

    auto secret_str = QString((char *) encoded);
    ui->secretEdit->setText(secret_str);
    clipboard.copyToClipboard(secret_str);
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
    uint8_t delete_user_password = nm::instance()->get_status().delete_user_password;
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
    nm::instance()->write_config(
        ui->numLockComboBox->currentIndex() - 1,
        ui->capsLockComboBox->currentIndex() - 1,
        ui->scrollLockComboBox->currentIndex() - 1,
        ui->enableUserPasswordCheckBox->isChecked(),
        ui->deleteUserPasswordCheckBox->isChecked() &&
        ui->enableUserPasswordCheckBox->isChecked(),
        auth_admin.getTempPassword().toLatin1().data()
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

void MainWindow::getTOTPDialog(int slot) {
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
  if (isHOTP) {
    res = libada::i()->eraseHOTPSlot(slotNo, auth_admin.getTempPassword().toLatin1().data());
  } else {
    res = libada::i()->eraseTOTPSlot(slotNo, auth_admin.getTempPassword().toLatin1().data());
  }
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
  int ret;

  int i;

  QString Slotname;

  ui->PWS_ComboBoxSelectSlot->clear();
  PWS_Access = FALSE;

  // Get active password slots
  const auto no_err = false; //TODO ERR_NO_ERROR == ret;
  if (libada::i()->isPasswordSafeUnlocked()) {
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

  tray.regenerateMenu();
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
    ui->PWS_EditPassword->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
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

#include "GUI/Authentication.h"
int MainWindow::getNextCode(uint8_t slotNumber) {
    const auto status = nm::instance()->get_status();
    QString tempPassword;

    if(status.enable_user_password != 0){
        if(!auth_user.authenticate()){
            return 0; //TODO throw
        }
        tempPassword = auth_user.getTempPassword();
    }
  //TODO authentication here
  if (slotNumber>=0x20){
      //FIXME set correct time on stick
    return libada::i()->getTOTPCode(slotNumber - 0x20, tempPassword.toLatin1().data());
  } else {
    return libada::i()->getHOTPCode(slotNumber - 0x10, tempPassword.toLatin1().data());
  }
  return 0;
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
  if (checked && libada::i()->is_nkpro_07_rtm1()) {
    bool answer = csApplet()->yesOrNoBox(tr("To handle this functionality "
                                                    "application will keep your user PIN in memory. "
                                                    "Do you want to continue?"), false);
    ui->enableUserPasswordCheckBox->setChecked(answer);
  }
}

void MainWindow::startLockDeviceAction() {

  PasswordSafeEnabled = FALSE;

  tray.regenerateMenu();
  tray.showTrayMessage("Nitrokey App", tr("Device has been locked"), INFORMATION, TRAY_MSG_TIMEOUT);
}

void MainWindow::updateProgressBar(int i) {
  ui->progressBar->setValue(i);
  if(i == 100){
    QTimer::singleShot(1000, [&](){
      ui->progressBar->hide();
    });
  }
}
