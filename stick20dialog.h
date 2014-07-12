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

#ifndef STICK20DIALOG_H
#define STICK20DIALOG_H

#include <QDialog>
#include "device.h"

namespace Ui {
class Stick20Dialog;
}

class Stick20Dialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit Stick20Dialog(QWidget *parent = 0);
    ~Stick20Dialog();

    Device *cryptostick;

private slots:
    void on_buttonBox_accepted();

    void on_comboBox_currentIndexChanged(int index);

    void on_checkBox_toggled(bool checked);

    void on_checkBox_Matrix_toggled(bool checked);

    void on_PasswordEdit_textChanged(const QString &arg1);

private:
    Ui::Stick20Dialog *ui;

    void InitEnterPasswordGui(char *PasswordKind);
    void InitNoPasswordGui(void);

};

#endif // STICK20DIALOG_H
