/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2014-06-02
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

#include "stick20lockfirmwaredialog.h"
#include "ui_stick20lockfirmwaredialog.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

  stick20LockFirmwareDialog

  Constructor stick20LockFirmwareDialog

  Reviews
  Date      Reviewer        Info


*******************************************************************************/

stick20LockFirmwareDialog::stick20LockFirmwareDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::stick20LockFirmwareDialog) {
  ui->setupUi(this);
}

stick20LockFirmwareDialog::~stick20LockFirmwareDialog() { delete ui; }
