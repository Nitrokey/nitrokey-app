/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
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

#ifndef STICK20HIDDENVOLUMEDIALOG_H
#define STICK20HIDDENVOLUMEDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include "libada.h"

namespace Ui {
class stick20HiddenVolumeDialog;
}
typedef struct {
    unsigned char SlotNr_u8;
    unsigned char StartBlockPercent_u8;
    unsigned char EndBlockPercent_u8;
    unsigned char HiddenVolumePassword_au8[MAX_HIDDEN_VOLUME_PASSOWORD_SIZE + 1] = {};
} HiddenVolumeSetup_tst;

class stick20HiddenVolumeDialog : public QDialog {
  Q_OBJECT
public :
  explicit stick20HiddenVolumeDialog(QWidget *parent = 0);
  ~stick20HiddenVolumeDialog();
  HiddenVolumeSetup_tst HV_Setup_st;

private:
  int GetCharsetSpace(unsigned char *Password, size_t size);
  double GetEntropy(unsigned char *Password, size_t size);

  uint8_t HighWatermarkMin;
  uint8_t HighWatermarkMax;
  int sd_size_GB;

  void setHighWaterMarkText(void);
  void on_rd_unit_clicked(QString text);

private slots:
  void on_ShowPasswordCheckBox_toggled(bool checked);

  void on_buttonBox_clicked(QAbstractButton *button);

  void on_HVPasswordEdit_textChanged(const QString &arg1);

  void on_rd_percent_clicked();
  void on_rd_MB_clicked();
  void on_rd_GB_clicked();

private:
  Ui::stick20HiddenVolumeDialog *ui;
};

#endif // STICK20HIDDENVOLUMEDIALOG_H
