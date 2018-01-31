/*
 * Author: Copyright (C)  Rudolf Boeddeker  Date: 2014-04-12
 * Copyright (c) 2014-2018 Nitrokey UG
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


#include "src/GUI/ManageWindow.h"
#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "libada.h"
#include "libnitrokey/include/NitrokeyManager.h"
#include "libnitrokey/include/DeviceCommunicationExceptions.h"
using nm = nitrokey::NitrokeyManager;

using namespace AboutDialogUI;

static const int invalid_value = 99;

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent), ui(nullptr) {
  ui = std::make_shared<Ui::AboutDialog>();
  ui->setupUi(this);

  connect(&worker_thread, SIGNAL(started()), &worker, SLOT(fetch_device_data()));
  connect(&worker, SIGNAL(finished(bool)), this, SLOT(update_device_slots(bool)));
  worker.moveToThread(&worker_thread);
  worker_thread.start();

  QPixmap image(":/images/splash.png");
  QPixmap small_img = image.scaled(346, 80, Qt::KeepAspectRatio, Qt::FastTransformation);
  QPixmap warning(":/images/warning.png");
//  QPixmap small_warning = warning.scaled(50, 50, Qt::KeepAspectRatio, Qt::FastTransformation);
  QPixmap info_img(":/images/info-icon.png");
  QPixmap small_info = info_img.scaled(15, 15, Qt::KeepAspectRatio, Qt::FastTransformation);

  ui->info_icon->setPixmap(small_info);
  ui->warning_sign->setPixmap(warning);
  ui->IconLabel->setPixmap(small_img);


  ui->admin_retry_label->setDisabled(true);
  ui->user_retry_label->setDisabled(true);
  ui->l_storage_capacity->setDisabled(true);
  ui->firmwareLabel->setDisabled(true);
  ui->serialEdit->setDisabled(true);

  auto string = QString(GUI_VERSION);
  ui->VersionLabel->setText(string);

  ui->ButtonStickStatus->hide();
  hideWarning();
  ui->not_erased_sd_label->hide();
  ui->newsd_label->hide();

  if (libada::i()->isDeviceConnected()) {
    if (libada::i()->isStorageDeviceConnected()) {
      showStick20Configuration();
    } else {
      showStick10Configuration();
    }
  } else {
    showNoStickFound();
  }

  fixWindowGeometry();

  ManageWindow::bringToFocus(this);
}

AboutDialog::~AboutDialog() {
  worker_thread.quit();
  worker_thread.wait();
}

void AboutDialog::on_ButtonOK_clicked() { done(TRUE); }

#include <QFile>
#include <QTextStream>
#include <QWindow>
#include <QTextEdit>
#include <libnitrokey/include/stick20_commands.h>
#include "licensedialog.h"


void AboutDialog::on_btn_3rdparty_clicked(){
  QString line;
  std::vector<QString> filenames {"General","Nitrokey-App",
                                  "Qt-framework",
                                  "libnitrokey","cppcodec"};
  for (auto filename : filenames) {
    QFile file(":license/" + filename);
    if (!file.exists()) {
      return;
    }
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      QTextStream stream(&file);
      if (!line.isEmpty()) line += "\n\n";
      auto a = 60;
      line += filename.leftJustified(a/2, '=').rightJustified(a, '=') + "\n\n\n";
      line += stream.readAll() + "\n";
    }
    file.close();
  }

  LicenseDialog l;
  l.setText(line);
  l.setModal(true);
  l.show();
  l.exec();
}

void AboutDialog::showStick20Configuration(void) {
  showPasswordCounters();
  showStick20Menu();

  fixWindowGeometry();
}

void AboutDialog::showStick10Configuration(void) {
  showPasswordCounters();
  hideStick20Menu();
  fixWindowGeometry();
}

void AboutDialog::fixWindowGeometry() {
  updateGeometry();
}

void AboutDialog::hideWarning(void) {
  ui->label_12->hide();
  ui->label_13->hide();
  ui->warning_sign->hide();
  fixWindowGeometry();
}

void AboutDialog::showWarning(void) {
  ui->label_12->show();
  ui->label_13->show();
  ui->warning_sign->show();
  fixWindowGeometry();
}

void AboutDialog::hideStick20Menu(void) {
  hideWarning();
  setStorageLabelsVisible(false);

  fixWindowGeometry();
}

void AboutDialog::setStorageLabelsVisible(bool v) {
  auto labels = {
      ui->hidden_volume_label,
      ui->static_storage_info,
      ui->label_8,
      ui->label_9,
      ui->label_10,
      ui->label_11,
      ui->unencrypted_volume_label,
      ui->encrypted_volume_label,
  };

  for (auto l: labels){
    l->setVisible(v);
  }
}

void AboutDialog::showStick20Menu(void) {
  setStorageLabelsVisible(true);
}

void AboutDialog::hidePasswordCounters(void) {
  ui->label_2->hide();
  ui->user_retry_label->hide();
  ui->admin_retry_label->hide();
  ui->label1->hide();
  ui->label2->hide();
  fixWindowGeometry();
}

void AboutDialog::showPasswordCounters(void) {
  ui->label_2->show();
  ui->user_retry_label->show();
  ui->admin_retry_label->show();
  ui->label1->show();
  ui->label2->show();
  fixWindowGeometry();
}
/*******************************************************************************

  showNoStickFound

  Changes
  Date      Author        Info
  23.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void AboutDialog::showNoStickFound(void) {
  QString OutputText;

  hideStick20Menu();
  hidePasswordCounters();
  OutputText.append(QString(tr("No active Nitrokey\n\n")));

  ui->DeviceStatusLabel->setText(OutputText);

  ui->firmwareLabel->setText("-");
  ui->serialEdit->setText("-");
  fixWindowGeometry();
}

void AboutDialog::on_ButtonStickStatus_clicked() {
}

void Worker::fetch_device_data() {
  QMutexLocker lock(&mutex);
  if (!libada::i()->isDeviceConnected()) {
    emit finished(false);
    return;
  }

  try{
    devdata.majorFirmwareVersion = libada::i()->getMajorFirmwareVersion();
    devdata.minorFirmwareVersion = libada::i()->getMinorFirmwareVersion();

    devdata.storage.active = libada::i()->isStorageDeviceConnected();
    if (devdata.storage.active ) {
      devdata.storage.sdcard.size_GB = libada::i()->getStorageSDCardSizeGB();
      auto st = nm::instance()->get_status_storage(); //FIXME use libada?
      devdata.storage.volume_active.plain = st.VolumeActiceFlag_st.unencrypted;
      devdata.storage.volume_active.encrypted = st.VolumeActiceFlag_st.encrypted;
      devdata.storage.volume_active.hidden = st.VolumeActiceFlag_st.hidden;
      devdata.storage.volume_active.plain_RW = st.ReadWriteFlagUncryptedVolume_u8;
      devdata.storage.volume_active.encrypted_RW = st.ReadWriteFlagCryptedVolume_u8;
      devdata.storage.volume_active.hidden_RW = st.ReadWriteFlagHiddenVolume_u8;
      devdata.storage.keys_initiated = !st.StickKeysNotInitiated;
      devdata.storage.sdcard.is_new =  st.NewSDCardFound_st.NewCard; // st.NewSDCardFound_u8;
      devdata.storage.sdcard.filled_with_random = st.SDFillWithRandomChars_u8;
      devdata.storage.sdcard.id = st.ActiveSD_CardID_u32;
      devdata.storage.smartcard_id = st.ActiveSmartCardID_u32;
      devdata.storage.firmware_locked = st.FirmwareLocked_u8;
      devdata.passwordRetryCount = st.AdminPwRetryCount;
      devdata.userPasswordRetryCount = st.UserPwRetryCount;
    } else {
      devdata.passwordRetryCount = libada::i()->getAdminPasswordRetryCount();
      devdata.userPasswordRetryCount = libada::i()->getUserPasswordRetryCount();
    }
    devdata.cardSerial = libada::i()->getCardSerial();

  }
  catch (LongOperationInProgressException &e){
    devdata.storage.long_operation.status = true;
    devdata.storage.long_operation.progress = e.progress_bar_value;
  }
  catch (DeviceSendingFailure &e){
    devdata.comm_error = true;
  }
  emit finished(true);
}


void AboutDialog::update_device_slots(bool connected) {
  fixWindowGeometry();

  if(!connected){
    showNoStickFound();
    return;
  }

  QMutexLocker lock(&worker.mutex);

  QString OutputText;

  if (worker.devdata.storage.long_operation.status){
    OutputText.append(QString(tr("      *** Clearing data in progress ***")).append("\n"));
  }

  if (worker.devdata.comm_error) {
    OutputText.append(QString(tr("      *** Communication error ***")).append("\n"));
  }

  if (worker.devdata.comm_error || worker.devdata.storage.long_operation.status){
    showNoStickFound();
    ui->DeviceStatusLabel->setText(OutputText);
    fixWindowGeometry();
    return;
  }

  ui->admin_retry_label->setText(QString::number(worker.devdata.passwordRetryCount));
  ui->user_retry_label->setText(QString::number(worker.devdata.userPasswordRetryCount));

  if (worker.devdata.storage.active) {
    QString capacity_text = QString(tr("%1 GB")).arg(worker.devdata.storage.sdcard.size_GB);
    ui->l_storage_capacity->setText(capacity_text);


    if (!worker.devdata.storage.keys_initiated) {
      showWarning();
    } else {
      hideWarning();
    }

    if (worker.devdata.storage.firmware_locked) {
      OutputText.append(QString(tr("      *** Firmware is locked ***")).append("\n"));
    }

    ui->newsd_label->setVisible(worker.devdata.storage.sdcard.is_new);
    ui->not_erased_sd_label->setVisible(!worker.devdata.storage.sdcard.filled_with_random);

    auto read_write_active = worker.devdata.storage.volume_active.plain_RW == 0; //FIXME correct variable naming in lib
    ui->unencrypted_volume_label->setText(read_write_active ? tr("READ/WRITE") : tr("READ ONLY"));

    if (worker.devdata.storage.volume_active.hidden) {
      ui->hidden_volume_label->setText(tr("Active"));
    } else {
      ui->hidden_volume_label->setText(tr("Not active"));
      ui->encrypted_volume_label->setText(
          worker.devdata.storage.volume_active.encrypted ? tr("Active") : tr("Not active"));
    }

    ui->sd_id_label->setText(
        QString("0x").append(QString::number(worker.devdata.storage.sdcard.id, 16)));

    if (0 == worker.devdata.storage.sdcard.id) {
      OutputText.append(QString(tr("\nSD card is not accessible\n\n")));
    }
  }
  ui->firmwareLabel->setText(QString::number(worker.devdata.majorFirmwareVersion)
                                 .append(".")
                                 .append(QString::number(worker.devdata.minorFirmwareVersion)));

  ui->serialEdit->setText(QString::fromStdString(worker.devdata.cardSerial).trimmed());


  if (0 == worker.devdata.storage.smartcard_id) {
    ui->serialEdit->setText("-");
    OutputText.append(QString(tr("\nSmartcard is not accessible\n\n")));
  }

  if (invalid_value == worker.devdata.userPasswordRetryCount) {
    OutputText.append(QString(tr("No connection\nPlease retry")));
  }
  if (!OutputText.isEmpty()) {
    ui->DeviceStatusLabel->setText(OutputText);
  }

  ui->admin_retry_label->setEnabled(true);
  ui->user_retry_label->setEnabled(true);
  ui->l_storage_capacity->setEnabled(true);
  ui->firmwareLabel->setEnabled(true);
  ui->serialEdit->setEnabled(true);
  
  fixWindowGeometry();
}
