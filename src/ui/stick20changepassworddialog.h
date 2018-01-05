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


#ifndef DIALOGCHANGEPASSWORD_H
#define DIALOGCHANGEPASSWORD_H

 enum class PasswordKind {
     USER,
     ADMIN,
     RESET_USER,
     UPDATE
 };

#include <QDialog>

namespace Ui {
class DialogChangePassword;
}

class DialogChangePassword : public QDialog {
  Q_OBJECT
  public :
  // FIXME extract constant to global config
    const static int minimumPasswordLengthUser = 6;
    const static int minimumPasswordLengthAdmin = 8;
    const static int minimumPasswordLengthFirmware = 6;
    int minimumPasswordLength = {};

  explicit DialogChangePassword(QWidget *parent, PasswordKind _kind);
    ~DialogChangePassword();

    void InitData(void);
    virtual int exec() override;
//
  signals:
    void UserPinLocked();
//    void error(int errcode);

private slots:
  void on_checkBox_clicked(bool checked);

private:
  Ui::DialogChangePassword *ui;
  PasswordKind kind;
  void _changePassword();
  void accept(void) override;
  void UpdatePasswordRetry(void);
  void clearFields();
  void fixWindowGeometry();
  void UI_deviceNotInitialized() const;
  void moveWindowToCenter();
};

#endif // DIALOGCHANGEPASSWORD_H
