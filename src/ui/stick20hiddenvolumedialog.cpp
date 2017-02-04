/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2014-04-13
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

#include "stick20hiddenvolumedialog.h"
#include "math.h"
#include "mcvs-wrapper.h"
#include "nitrokey-applet.h"
#include "ui_stick20hiddenvolumedialog.h"
#include "src/utils/bool_values.h"



stick20HiddenVolumeDialog::stick20HiddenVolumeDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::stick20HiddenVolumeDialog) {
  ui->setupUi(this);

  SdCardHighWatermark_Read_Min = 0;
  SdCardHighWatermark_Read_Max = 100;
  SdCardHighWatermark_Write_Min = 0;
  SdCardHighWatermark_Write_Max = 100;

  HV_Setup_st.SlotNr_u8 = 0;
  HV_Setup_st.StartBlockPercent_u8 = 70;
  HV_Setup_st.EndBlockPercent_u8 = 90;
  HV_Setup_st.HiddenVolumePassword_au8[0] = 0;

  ui->comboBox->setCurrentIndex(HV_Setup_st.SlotNr_u8);

  //  ui->StartBlockSpin->setMaximum(89);
  //  ui->StartBlockSpin->setMinimum(10);
  ui->StartBlockSpin->setValue(HV_Setup_st.StartBlockPercent_u8);

  //  ui->EndBlockSpin->setMaximum(90);
  //  ui->EndBlockSpin->setMinimum(11);
  ui->EndBlockSpin->setValue(HV_Setup_st.EndBlockPercent_u8);

  ui->HVPasswordEdit->setMaxLength(MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
  ui->HVPasswordEdit->setText("");

  ui->HVPasswordEdit_2->setMaxLength(MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
  ui->HVPasswordEdit_2->setText("");

  ui->HVPasswordEdit->setFocus();

  on_HVPasswordEdit_textChanged("");

  on_rd_percent_clicked();
  ui->rd_percent->setChecked(true);
  //gethighwater
  setHighWaterMarkText();
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

    if (8 > strlen(ui->HVPasswordEdit->text().toLatin1().data())) {
        csApplet()->warningBox(tr("Your password is too short. Use at least 8 characters."));
      return;
    }

    if (ui->HVPasswordEdit->text().toLatin1() != ui->HVPasswordEdit_2->text().toLatin1()) {
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

    auto p = ui->HVPasswordEdit->text().toLatin1().constData(); //FIXME use proper copy
    strncpy((char *) HV_Setup_st.HiddenVolumePassword_au8, p, ui->HVPasswordEdit->text().toLatin1().length());
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
  Entropy = GetEntropy((unsigned char *)arg1.toLatin1().data(), Len);

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
  HighWatermarkMin = SdCardHighWatermark_Write_Min;
  HighWatermarkMax = SdCardHighWatermark_Write_Max;

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

  // Set valid input range
  //  ui->StartBlockSpin->setMaximum(HighWatermarkMax - 1);
  //  ui->StartBlockSpin->setMinimum(HighWatermarkMin);
  ui->StartBlockSpin->setValue(HV_Setup_st.StartBlockPercent_u8);

  //  ui->EndBlockSpin->setMaximum(HighWatermarkMax);
  //  ui->EndBlockSpin->setMinimum(HighWatermarkMin + 1);
  ui->EndBlockSpin->setValue(HV_Setup_st.EndBlockPercent_u8);
  int sd_size_GB = libada::i()->getStorageSDCardSizeGB();
  ui->l_sd_size->setText(tr("Storage capacity: %1GB").arg(sd_size_GB));
  ui->l_rounding_info->setText(ui->l_rounding_info->text().arg((sd_size_GB * 1024 / 100)));
}

static char last = '%';
static double i_start_MB = 0;
static double i_end_MB = 0;

void stick20HiddenVolumeDialog::on_rd_unit_clicked(QString text) {
  size_t sd_size_MB = libada::i()->getStorageSDCardSizeGB() * 1024;

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

  char current = (int)text.data()[0].toLatin1();

  switch (current) {
  case 'M':
    ui->StartBlockSpin->setValue(i_start_MB);
    ui->EndBlockSpin->setValue(i_end_MB);
    break;
  case 'G':
    ui->StartBlockSpin->setValue(i_start_MB / 1024.0);
    ui->EndBlockSpin->setValue(i_end_MB / 1024.0);
    break;
  case '%':
    ui->StartBlockSpin->setValue(100.0 * i_start_MB / sd_size_MB);
    ui->EndBlockSpin->setValue(100.0 * i_end_MB / sd_size_MB);
    break;
  default:
    break;
  }

  last = current;
}

void stick20HiddenVolumeDialog::on_rd_MB_clicked() { on_rd_unit_clicked("MB"); }

void stick20HiddenVolumeDialog::on_rd_GB_clicked() { on_rd_unit_clicked("GB"); }

void stick20HiddenVolumeDialog::on_rd_percent_clicked() { on_rd_unit_clicked("%"); }
