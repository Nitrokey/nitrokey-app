/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
*
* This file is part of GPF Crypto Stick 2
*
* GPF Crypto Stick 2  is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* GPF Crypto Stick is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GPF Crypto Stick. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DIALOGCHANGEPASSWORD_H
#define DIALOGCHANGEPASSWORD_H

#include <QDialog>
#include "device.h"

namespace Ui {
class DialogChangePassword;
}

class DialogChangePassword : public QDialog
{
    Q_OBJECT
    
public:
    explicit DialogChangePassword(QWidget *parent = 0);
    ~DialogChangePassword();

    void InitData(void);

    int PasswordKind;
    
    Device *cryptostick;

private slots:
    void on_checkBox_clicked(bool checked);

private:
    Ui::DialogChangePassword *ui;
    void accept();
    void SendNewPassword(void);
    int  CheckResponse(bool NoStopFlag);
};

#endif // DIALOGCHANGEPASSWORD_H
