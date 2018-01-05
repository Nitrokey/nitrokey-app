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

#include "stick20debugdialog.h"
#include "ui_stick20debugdialog.h"


DebugDialog::DebugDialog(QWidget *parent) : QDialog(parent), ui(new Ui::DebugDialog) {
  ui->setupUi(this);

  ui->TextGUI->clear();
}

DebugDialog::~DebugDialog() {
  delete ui;
}


void DebugDialog::on_DebugData(QString message) {
  message[message.size()-1] = ' ';
  ui->TextGUI->appendPlainText(message);
//  ui->TextGUI->appendPlainText(message.trimmed());
}
