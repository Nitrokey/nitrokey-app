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

#ifndef DIALOGCHANGEPASSWORD_H
#define DIALOGCHANGEPASSWORD_H

#include "device.h"
#include <QDialog>

namespace Ui {
class DialogChangePassword;
}

class DialogChangePassword : public QDialog {
  Q_OBJECT public : const static int minimumPasswordLength =
                        6; // FIXME extract constant to global config
  explicit DialogChangePassword(QWidget *parent = 0);
  ~DialogChangePassword();

  void InitData(void);

  int PasswordKind;

  Device *cryptostick;

private slots:
  void on_checkBox_clicked(bool checked);

private:
  Ui::DialogChangePassword *ui;
  void accept(void);
  bool SendNewPassword(void);
  void Stick10ChangePassword(void);
  void ResetUserPassword(void);
  void ResetUserPasswordStick10(void);
  void Stick20ChangeUpdatePassword(void);
  int CheckResponse(bool NoStopFlag);
  void clearFields();
};

#endif // DIALOGCHANGEPASSWORD_H
