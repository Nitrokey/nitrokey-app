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

#ifndef STICK20HIDDENVOLUMEDIALOG_H
#define STICK20HIDDENVOLUMEDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include "libada.h"
#include <atomic>

namespace Ui {
class stick20HiddenVolumeDialog;
}
typedef struct {
    unsigned char SlotNr_u8 = 0;
    unsigned char StartBlockPercent_u8 = 70;
    unsigned char EndBlockPercent_u8 = 90;
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
  void on_StartBlockSpin_valueChanged(double i);
  void on_EndBlockSpin_valueChanged(double i);

private:
  Ui::stick20HiddenVolumeDialog *ui;

  void set_spins_min_max(const double min, const double max, const double step);
  char last = '%';
  double i_start_MB = 0;
  double i_end_MB = 0;
  double current_min = 0;
  double current_max = 100;
  double current_step = 1;

  std::atomic_bool cancel_BlockSpin_event_propagation {false};

};

#endif // STICK20HIDDENVOLUMEDIALOG_H
