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
    const static int minimumPasswordLength =
                        6; // FIXME extract constant to global config
  explicit DialogChangePassword(QWidget *parent, PasswordKind _kind);
  ~DialogChangePassword();

  void InitData(void);
  virtual int exec() override;


private slots:
  void on_checkBox_clicked(bool checked);

private:
  PasswordKind kind;
  Ui::DialogChangePassword *ui;
  bool _changePassword();
  void accept(void);
  void UpdatePasswordRetry(void);
  void clearFields();
  void UI_deviceNotInitialized() const;
  void moveWindowToCenter();
};

#endif // DIALOGCHANGEPASSWORD_H
