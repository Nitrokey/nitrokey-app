/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
 * Copyright (c) 2013-2018 Nitrokey UG
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

#include <src/core/ThreadWorker.h>
#include <libnitrokey/NitrokeyManager.h>
#include "stick20hiddenvolumedialog.h"
#include "math.h"
#include "mcvs-wrapper.h"
#include "nitrokey-applet.h"
#include "ui_stick20hiddenvolumedialog.h"
#include "src/utils/bool_values.h"



stick20HiddenVolumeDialog::stick20HiddenVolumeDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::stick20HiddenVolumeDialog) {
  ui->setupUi(this);

  ui->comboBox->setCurrentIndex(HV_Setup_st.SlotNr_u8);
  ui->StartBlockSpin->setValue(HV_Setup_st.StartBlockPercent_u8);
  ui->EndBlockSpin->setValue(HV_Setup_st.EndBlockPercent_u8);

  ui->HVPasswordEdit->setMaxLength(MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
  ui->HVPasswordEdit->setText("");

  ui->HVPasswordEdit_2->setMaxLength(MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
  ui->HVPasswordEdit_2->setText("");

  ui->HVPasswordEdit->setFocus();
  ui->HV_settings_groupBox->setEnabled(false);

  new ThreadWorker(
    []() -> Data {
      Data data;
      auto m = nitrokey::NitrokeyManager::instance();
      auto p = m->get_SD_usage_data();
      data["min"] = p.first;
      data["max"] = p.second;
      data["size"] = libada::i()->getStorageSDCardSizeGB();
      return data;
    },
    [this](Data data){
      HighWatermarkMin = (uint8_t) data["min"].toInt();
      HighWatermarkMax = (uint8_t) data["max"].toInt();
      sd_size_GB = data["size"].toInt();
      setHighWaterMarkText();
      on_rd_percent_clicked();
      ui->rd_percent->setChecked(true);
      ui->HV_settings_groupBox->setEnabled(true);
    }, this);
}

stick20HiddenVolumeDialog::~stick20HiddenVolumeDialog() { delete ui; }

void stick20HiddenVolumeDialog::on_ShowPasswordCheckBox_toggled(bool checked) {
  if (checked) {
    ui->HVPasswordEdit_2->setEchoMode(QLineEdit::Normal);
    ui->HVPasswordEdit->setEchoMode(QLineEdit::Normal);
  } else {
    ui->HVPasswordEdit_2->setEchoMode(QLineEdit::Password);
    ui->HVPasswordEdit->setEchoMode(QLineEdit::Password);
  }
}

void stick20HiddenVolumeDialog::on_buttonBox_clicked(QAbstractButton *button) {
  if (button == (QAbstractButton *) ui->buttonBox->button(QDialogButtonBox::Ok)) {
    on_rd_percent_clicked();
    ui->rd_percent->setChecked(true);

    auto password_byte_array = ui->HVPasswordEdit->text().toLatin1();
    if (8 > strlen(password_byte_array.constData())) {
        csApplet()->warningBox(tr("Your password is too short. Use at least 8 characters."));
      return;
    }

    if (ui->HVPasswordEdit->text() != ui->HVPasswordEdit_2->text()) {
        csApplet()->warningBox(tr("The passwords are not identical"));
      return;
    }

    HV_Setup_st.SlotNr_u8 = ui->comboBox->currentIndex();
    HV_Setup_st.StartBlockPercent_u8 = ui->StartBlockSpin->value();
    HV_Setup_st.EndBlockPercent_u8 = ui->EndBlockSpin->value();

    if (HV_Setup_st.StartBlockPercent_u8 >= HV_Setup_st.EndBlockPercent_u8) {
        csApplet()->warningBox(tr("Wrong size of hidden volume"));
      return;
    }

    if (HV_Setup_st.StartBlockPercent_u8 < HighWatermarkMin ||
        HV_Setup_st.EndBlockPercent_u8 > HighWatermarkMax) {
      csApplet()->warningBox(tr("Hidden volume not positioned in unwritten space. Please set your "
                              "volume between %1% and %2% of total SD card size.")
                               .arg(HighWatermarkMin)
                               .arg(HighWatermarkMax));
      return;
    }

    auto p = password_byte_array.constData(); //FIXME use proper copy
    strncpy((char *) HV_Setup_st.HiddenVolumePassword_au8, p, password_byte_array.length());
    HV_Setup_st.HiddenVolumePassword_au8[MAX_HIDDEN_VOLUME_PASSOWORD_SIZE] = 0;
    done(true);
  } else if (button == (QAbstractButton *) ui->buttonBox->button(QDialogButtonBox::Cancel)) {
    done(false);
  }
}

// Based on
// http://www.emoticode.net/c/optimized-f-the-shannf-the-shannon-entropy-algorithm.html
// (site not working as of 2016.08)

struct charset_space {
  int CharSpace;
  int HasLowerAlpha : 1;
  int HasUpperAlpha : 1;
  int HasGermanChars : 1;
  int HasNumber : 1;
  int HasSpecialChars1 : 1;
  int HasSpecialChars2 : 1;
  int HasSpecialChars3 : 1;
  void clear() { memset(this, 0, sizeof(*this)); }
} c;

int stick20HiddenVolumeDialog::GetCharsetSpace(unsigned char *Password, size_t size) {
  size_t i;
  c.clear();
  for (i = 0; i < size; i++) {
    if ((FALSE == c.HasLowerAlpha) && (0 != strchr("abcdefghijklmnopqrstuvwxyz", Password[i]))) {
      c.CharSpace += 26;
      c.HasLowerAlpha = TRUE;
    }
    if ((FALSE == c.HasUpperAlpha) && (0 != strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ", Password[i]))) {
      c.CharSpace += 26;
      c.HasUpperAlpha = TRUE;
    }
    if ((FALSE == c.HasGermanChars) && (0 != strchr("öäüÖÄÜß", Password[i]))) {
      c.CharSpace += 7;
      c.HasGermanChars = TRUE;
    }
    if ((FALSE == c.HasNumber) && (0 != strchr("0123456789", Password[i]))) {
      c.CharSpace += 10;
      c.HasNumber = TRUE;
    }
    if ((FALSE == c.HasSpecialChars1) && (0 != strchr("!\"§$%&/()=?'", Password[i]))) {
      c.CharSpace += 11;
      c.HasSpecialChars1 = TRUE;
    }
    if ((FALSE == c.HasSpecialChars2) && (0 != strchr("',.-#+´;:_*<>^°`", Password[i]))) {
      c.CharSpace += 16;
      c.HasSpecialChars2 = TRUE;
    }
    if ((FALSE == c.HasSpecialChars3) && (0 != strchr("~[{]}\\|€@", Password[i]))) {
      c.CharSpace += 9;
      c.HasSpecialChars3 = TRUE;
    }
  }

  return (c.CharSpace);
}

double stick20HiddenVolumeDialog::GetEntropy(unsigned char *Password, size_t size) {
  double Entropy = 0.0;
  int CharsetSpace;

  CharsetSpace = GetCharsetSpace(Password, size);
  if (CharsetSpace == 0)
    return 0;
  // Entropy by CharsetSpace * size
  Entropy = (double)size * (log((double)CharsetSpace) / log(2.0));
  return (Entropy);
}

int password_human_strength(double entropy) {
  if (entropy < 28)
    return 0;
  if (entropy < 36)
    return 1;
  if (entropy < 60)
    return 2;
  if (entropy < 128)
    return 3;
  return 4;
}

void stick20HiddenVolumeDialog::on_HVPasswordEdit_textChanged(const QString &arg1) {
  int Len;
  double Entropy;
  QString labels[] = {tr("Very Weak"), tr("Weak"), tr("Medium"), tr("Strong"), tr("Very Strong")};

  Len = arg1.length();
  auto byteArray = arg1.toLatin1();
  Entropy = GetEntropy((unsigned char *) byteArray.data(), Len);

  if (Entropy < 0) {
    Entropy = 0;
  }
  // == using human readable symbols, 127 bits
  //  ui->HVEntropieRealWords->setText(
  //      QString("%1").sprintf("using human readable symbols: %3.1lf", Entropy / 2.0));
  int strength = password_human_strength(Entropy);
  ui->password_strength_progressBar->setFormat(labels[strength]);
  ui->password_strength_progressBar->setValue(100.0 * Entropy / 64.0 / 2.0);
  //    ui->password_strength_progressBar->setValue(100*strength/4);
  ui->cb_lower_case->setChecked(c.HasLowerAlpha);
  ui->cb_upper_case->setChecked(c.HasUpperAlpha);
  ui->cb_numbers->setChecked(c.HasNumber);
  ui->cb_symbols->setChecked(c.HasSpecialChars1 || c.HasSpecialChars2 || c.HasSpecialChars3 ||
                             c.HasGermanChars);
  ui->cb_length->setChecked(Len > 12);
}

void stick20HiddenVolumeDialog::setHighWaterMarkText(void) {
  if (5 > HighWatermarkMin) // Set lower limit
  {
    HighWatermarkMin = 10;
  }
  if (90 < HighWatermarkMax) // Set higher limit
  {
    HighWatermarkMax = 90;
  }

  ui->HVSdCardHighWaterMark->setText(
      tr("The unwritten area available for hidden volume\nis between "
         "%1 % and %2 % of the storage size")
          .arg(HighWatermarkMin)
          .arg(HighWatermarkMax));

  ui->StartBlockSpin->setValue(HV_Setup_st.StartBlockPercent_u8);
  ui->EndBlockSpin->setValue(HV_Setup_st.EndBlockPercent_u8);
  ui->l_sd_size->setText(tr("Storage capacity: %1GB").arg(sd_size_GB));
  auto rounding_info = ui->l_rounding_info->text().arg((sd_size_GB * 1024 / 100));
  ui->l_rounding_info->setText(rounding_info);
  ui->StartBlockSpin->setAccessibleDescription(rounding_info);
  ui->EndBlockSpin->setAccessibleDescription(rounding_info);
}



void stick20HiddenVolumeDialog::on_rd_unit_clicked(QString text) {
  const size_t sd_size_MB = sd_size_GB * 1024u;

  double current_block_start = ui->StartBlockSpin->value();
  double current_block_end = ui->EndBlockSpin->value();

  if (i_start_MB == 0) {
    i_start_MB = sd_size_MB * current_block_start / 100.0;
    i_end_MB = sd_size_MB * current_block_end / 100.0;
  }

  QString start = tr("Start hidden volume at %1 of the encrypted storage:");
  QString end = tr("End hidden volume at %1 of the encrypted storage:");
  ui->l_sd_start->setText(start.arg(text));
  ui->l_sd_end->setText(end.arg(text));
  ui->StartBlockSpin->setAccessibleName(start.arg(text));
  ui->EndBlockSpin->setAccessibleName(end.arg(text));

  switch (last) {
  case 'M':
    i_start_MB = current_block_start;
    i_end_MB = current_block_end;
    break;
  case 'G':
    i_start_MB = current_block_start * 1024;
    i_end_MB = current_block_end * 1024;
    break;
  case '%':
    i_start_MB = current_block_start * sd_size_MB / 100.0;
    i_end_MB = current_block_end * sd_size_MB / 100.0;
    break;
  default:
    break;
  }

  cancel_BlockSpin_event_propagation = true;

  char current = text.data()[0].toLatin1();
  switch (current) {
    case 'M':
      current_min = HighWatermarkMin * sd_size_MB / 100.;
      current_max = HighWatermarkMax * sd_size_MB / 100.;
      current_step = sd_size_MB/100.;
      set_spins_min_max(current_min, current_max, current_step);
      ui->StartBlockSpin->setValue(i_start_MB);
      ui->EndBlockSpin->setValue(i_end_MB);
      break;
    case 'G':
      current_min = HighWatermarkMin * sd_size_GB / 100.;
      current_max = HighWatermarkMax * sd_size_GB / 100.;
      current_step = sd_size_GB/100.;
      set_spins_min_max(current_min, current_max, current_step);
      ui->StartBlockSpin->setValue(i_start_MB / 1024.0);
      ui->EndBlockSpin->setValue(i_end_MB / 1024.0);
      break;
    case '%':
      current_min = HighWatermarkMin;
      current_max = HighWatermarkMax;
      current_step = 1;
      set_spins_min_max(current_min, current_max, current_step);
      ui->StartBlockSpin->setValue(100.0 * i_start_MB / sd_size_MB);
      ui->EndBlockSpin->setValue(100.0 * i_end_MB / sd_size_MB);
      break;
    default:
      break;
  }

  cancel_BlockSpin_event_propagation = false;
  last = current;
}

void stick20HiddenVolumeDialog::set_spins_min_max(const double min, const double max, const double step) {
  ui->StartBlockSpin->setSingleStep(step);
  ui->StartBlockSpin->setRange(min,max);
  ui->EndBlockSpin->setSingleStep(step);
  ui->EndBlockSpin->setRange(min,max);
}

void stick20HiddenVolumeDialog::on_EndBlockSpin_valueChanged(double i){
  Q_UNUSED(i)
  if (cancel_BlockSpin_event_propagation) return;
  cancel_BlockSpin_event_propagation = true;
  auto start_val = ui->StartBlockSpin->value();
  auto current_val = ui->EndBlockSpin->value();
  if(current_val < start_val || current_val - start_val < current_step){
    ui->EndBlockSpin->setValue(start_val + current_step);
  } else if(current_val > current_max){
    ui->EndBlockSpin->setValue(current_max);
  }
  cancel_BlockSpin_event_propagation = false;
}

void stick20HiddenVolumeDialog::on_StartBlockSpin_valueChanged(double i){
  Q_UNUSED(i)
  if (cancel_BlockSpin_event_propagation) return;
  cancel_BlockSpin_event_propagation = true;
  auto end_val = ui->EndBlockSpin->value();
  auto current_val = ui->StartBlockSpin->value();
  if(current_val > end_val || end_val - current_val < current_step){
    ui->StartBlockSpin->setValue(end_val - current_step);
  } else if(current_val < current_min){
    ui->StartBlockSpin->setValue(current_min);
  }
  cancel_BlockSpin_event_propagation = false;
}

void stick20HiddenVolumeDialog::on_rd_MB_clicked() { on_rd_unit_clicked("MB"); }

void stick20HiddenVolumeDialog::on_rd_GB_clicked() { on_rd_unit_clicked("GB"); }

void stick20HiddenVolumeDialog::on_rd_percent_clicked() { on_rd_unit_clicked("%"); }
