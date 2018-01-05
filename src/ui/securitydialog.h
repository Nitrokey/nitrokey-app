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

#ifndef SECURITYDIALOG_H
#define SECURITYDIALOG_H

#include <QDialog>

namespace Ui {
class securitydialog;
}

class securitydialog : public QDialog {
  Q_OBJECT public : explicit securitydialog(QWidget *parent = 0);
  ~securitydialog();

private slots:
  void on_ST_CheckBox_toggled(bool checked);

  void on_ST_OkButton_clicked();

private:
  Ui::securitydialog *ui;
};

#endif // SECURITYDIALOG_H
