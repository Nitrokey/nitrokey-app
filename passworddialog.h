/*
* Author: Copyright (C) Andrzej Surowiec 2012
*
*
* This file is part of GPF Crypto Stick.
*
* GPF Crypto Stick is free software: you can redistribute it and/or modify
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

#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>

#include "device.h"

namespace Ui {
class PasswordDialog;
}

class PasswordDialog : public QDialog
{
    Q_OBJECT
    
public:
    unsigned char password[50];
//    QByteArray  passwordString;
    Device *cryptostick;

    explicit PasswordDialog(bool ShowMatrix,QWidget *parent = 0);
    ~PasswordDialog();

    void init(char *text,int RetryCount);
    void getPassword(char *text);

private slots:
    void on_checkBox_toggled(bool checked);

    void on_checkBox_PasswordMatrix_toggled(bool checked);

    void on_buttonBox_accepted();

private:
    Ui::PasswordDialog *ui;
};

#endif // PASSWORDDIALOG_H
