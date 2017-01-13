/*
 * Author: Copyright (C)  Rudolf Boeddeker  Date: 2014-04-12
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


#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "stick20responsedialog.h"
#include "libada.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AboutDialog) {
  ui->setupUi(this);

  QPixmap image(":/images/splash.png");
  QPixmap small_img = image.scaled(346, 80, Qt::KeepAspectRatio, Qt::FastTransformation);
  QPixmap warning(":/images/warning.png");
//  QPixmap small_warning = warning.scaled(50, 50, Qt::KeepAspectRatio, Qt::FastTransformation);
  QPixmap info_img(":/images/info-icon.png");
  QPixmap small_info = info_img.scaled(15, 15, Qt::KeepAspectRatio, Qt::FastTransformation);

  ui->info_icon->setPixmap(small_info);
  ui->warning_sign->setPixmap(warning);
  ui->IconLabel->setPixmap(small_img);


  int majorFirmwareVersion = libada::i()->getMajorFirmwareVersion();
  int minorFirmwareVersion = libada::i()->getMinorFirmwareVersion();

  ui->IconLabel->setPixmap(small_img);
  ui->VersionLabel->setText(tr(GUI_VERSION));
  ui->firmwareLabel->setText(QString::number(majorFirmwareVersion)
                                 .append(".")
                                 .append(QString::number(minorFirmwareVersion)));

  auto cardSerial = libada::i()->getCardSerial();
  ui->serialEdit->setText(QString::fromStdString(cardSerial));

  ui->ButtonStickStatus->hide();

  if (libada::i()->isDeviceConnected()) {
    if (libada::i()->isStorageDeviceConnected()) {
      showStick20Configuration();
    } else {
      showStick10Configuration();
      ui->ButtonStickStatus->hide();
    }
  } else {
    showNoStickFound();
    // ui->ButtonStickStatus->hide();
  }

  this->adjustSize();
  this->updateGeometry();
}

AboutDialog::~AboutDialog() { delete ui; }

void AboutDialog::on_ButtonOK_clicked() { done(TRUE); }

/*******************************************************************************

  showStick20Configuration

  Changes
  Date      Author        Info
  02.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void AboutDialog::showStick20Configuration(void) {
  QString OutputText;
  bool ErrorFlag;
  ErrorFlag = FALSE;

  showPasswordCounters();
  showStick20Menu();

  auto storageInfoData = libada::i()->getStorageInfoData();

//  if (0 == HID_Stick20Configuration_st.ActiveSD_CardID_u32) {
//    OutputText.append(QString(tr("\nSD card is not accessible\n\n")));
//    ErrorFlag = TRUE;
//  }
//
//  if (0 == HID_Stick20Configuration_st.ActiveSmartCardID_u32) {
//    ui->serialEdit->setText("-");
//    OutputText.append(QString(tr("\nSmartcard is not accessible\n\n")));
//    ErrorFlag = TRUE;
//  }
//
//  if (TRUE == ErrorFlag) {
//    ui->DeviceStatusLabel->setText(OutputText);
//    return;
//  }
//
//  if (99 == HID_Stick20Configuration_st.UserPwRetryCount) {
//    OutputText.append(QString(tr("No connection\nPlease retry")));
//    ui->DeviceStatusLabel->setText(OutputText);
//    return;
//  }
//
//  if (TRUE == HID_Stick20Configuration_st.StickKeysNotInitiated) {
//    showWarning();
//  } else {
//    hideWarning();
//  }
//
//  if (TRUE == HID_Stick20Configuration_st.FirmwareLocked_u8) {
//    // OutputText.append (QString (tr ("      *** Firmware is locked ***
//    // ")).append ("\n"));
//  }
//
//  if (0 != (HID_Stick20Configuration_st.NewSDCardFound_u8 & 0x01)) {
//    ui->newsd_label->show();
//  } else {
//    ui->newsd_label->hide();
//  }
//
//  if (0 == (HID_Stick20Configuration_st.SDFillWithRandomChars_u8 & 0x01)) {
//    ui->not_erased_sd_label->show();
//  } else {
//    ui->not_erased_sd_label->hide();
//  }
//
//  if (READ_WRITE_ACTIVE == HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8) {
//    ui->sd_id_label->setText(tr("READ/WRITE"));
//  } else {
//    ui->sd_id_label->setText(tr("READ ONLY"));
//  }
//
//  if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_HIDDEN_VOLUME_BIT_PLACE))) {
//    ui->hidden_volume_label->setText(tr("Active"));
//  } else {
//    ui->hidden_volume_label->setText(tr("Not active"));
//    if (0 !=
//        (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_CRYPTED_VOLUME_BIT_PLACE))) {
//      ui->encrypted_volume_label->setText(tr("Active"));
//    } else {
//      ui->encrypted_volume_label->setText(tr("Not active"));
//    }
//  }

//  ui->sd_id_label->setText(
//      QString("0x").append(QString::number(HID_Stick20Configuration_st.ActiveSD_CardID_u32, 16)));

  ui->admin_retry_label->setText(QString::number(libada::i()->getPasswordRetryCount()));
  ui->user_retry_label->setText(QString::number(libada::i()->getUserPasswordRetryCount()));
//  ui->firmwareLabel->setText(
//      QString::number(HID_Stick20Configuration_st.VersionInfo_au8[0])
//          .append(".")
//          .append(QString::number(HID_Stick20Configuration_st.VersionInfo_au8[1])));
//  ui->serialEdit->setText(
//      QString("%1").sprintf("%08x", HID_Stick20Configuration_st.ActiveSmartCardID_u32));

//  ui->DeviceStatusLabel->setText(OutputText);

  if (libada::i()->isStorageDeviceConnected()){
    const size_t sd_size_GB = libada::i()->getStorageSDCardSizeGB();
    QString capacity_text = QString(tr("%1 GB")).arg(sd_size_GB);
    ui->l_storage_capacity->setText(capacity_text);
  }

  this->resize(0, 0);
  this->adjustSize();
  this->updateGeometry();
}

/*******************************************************************************

  showStick10Configuration

  Changes
  Date      Author        Info
  23.07.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void AboutDialog::showStick10Configuration(void) {
  showPasswordCounters();
  hideStick20Menu();

  ui->admin_retry_label->setText(QString::number(libada::i()->getPasswordRetryCount()));
  ui->user_retry_label->setText(QString::number(libada::i()->getUserPasswordRetryCount()));
  this->resize(0, 0);
  this->adjustSize();
  this->updateGeometry();
}

void AboutDialog::hideWarning(void) {
  ui->label_12->hide();
  ui->label_13->hide();
  ui->warning_sign->hide();
  this->resize(0, 0);
  this->adjustSize();
  this->updateGeometry();
}

void AboutDialog::showWarning(void) {
  ui->label_12->show();
  ui->label_13->show();
  ui->warning_sign->show();
  this->resize(0, 0);
  this->adjustSize();
  this->updateGeometry();
}

void AboutDialog::hideStick20Menu(void) {
  hideWarning();

  ui->hidden_volume_label->hide();
  ui->not_erased_sd_label->hide();
  ui->newsd_label->hide();
  ui->static_storage_info->hide();
  /*
  ui->label_8->hide();
  ui->label_9->hide();
  ui->label_10->hide();
  ui->label_11->hide();
  ui->sd_id_label->hide();
  ui->unencrypted_volume_label->hide();
  ui->encrypted_volume_label->hide();
  */
  this->resize(0, 0);
  this->adjustSize();
  this->updateGeometry();
}

void AboutDialog::showStick20Menu(void) {

  ui->hidden_volume_label->show();
  ui->unencrypted_volume_label->show();
  ui->encrypted_volume_label->show();
  ui->static_storage_info->show();
  /*
  ui->label_8->show();
  ui->label_9->show();
  ui->label_10->show();
  ui->label_11->show();
  ui->sd_id_label->show();
  this->resize(0,0);
  this->adjustSize ();
  this->updateGeometry();
  */
}

void AboutDialog::hidePasswordCounters(void) {
  ui->label_2->hide();
  ui->user_retry_label->hide();
  ui->admin_retry_label->hide();
  ui->label1->hide();
  ui->label2->hide();
  this->resize(0, 0);
  this->adjustSize();
  this->updateGeometry();
}

void AboutDialog::showPasswordCounters(void) {
  ui->label_2->show();
  ui->user_retry_label->show();
  ui->admin_retry_label->show();
  ui->label1->show();
  ui->label2->show();
  this->resize(0, 0);
  this->adjustSize();
  this->updateGeometry();
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
  this->resize(0, 0);
  this->adjustSize();
  this->updateGeometry();
}

void AboutDialog::on_ButtonStickStatus_clicked() {
}
