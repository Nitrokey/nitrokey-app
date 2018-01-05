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

#ifndef STICK20LOCKFIRMWAREDIALOG_H
#define STICK20LOCKFIRMWAREDIALOG_H

#include <QDialog>

namespace Ui {
class stick20LockFirmwareDialog;
}

class stick20LockFirmwareDialog : public QDialog {
  Q_OBJECT public : explicit stick20LockFirmwareDialog(QWidget *parent = 0);
  ~stick20LockFirmwareDialog();

private:
  Ui::stick20LockFirmwareDialog *ui;
};

#endif // STICK20LOCKFIRMWAREDIALOG_H
