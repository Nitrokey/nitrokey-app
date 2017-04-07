/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-12
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
  ui->TextGUI->appendPlainText(message.trimmed());
}
