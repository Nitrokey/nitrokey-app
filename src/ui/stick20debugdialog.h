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

#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#include <QDialog>

namespace Ui {
class DebugDialog;
}

class DebugDialog : public QDialog {
  Q_OBJECT public : explicit DebugDialog(QWidget *parent = 0);
  virtual ~DebugDialog();

  public slots:
    void on_DebugData(QString message);


private:
  Ui::DebugDialog *ui;

};

#endif // DEBUGDIALOG_H
