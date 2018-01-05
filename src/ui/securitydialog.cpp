/*
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

#include "securitydialog.h"
#include "ui_securitydialog.h"

securitydialog::securitydialog(QWidget *parent) : QDialog(parent), ui(new Ui::securitydialog) {
  ui->setupUi(this);
  ui->ST_OkButton->setDisabled(true);
}

securitydialog::~securitydialog() { delete ui; }

void securitydialog::on_ST_CheckBox_toggled(bool checked) {
  if (true == checked) {
    ui->ST_OkButton->setEnabled(true);
  } else {
    ui->ST_OkButton->setDisabled(true);
  }
}

void securitydialog::on_ST_OkButton_clicked() { done(true); }
