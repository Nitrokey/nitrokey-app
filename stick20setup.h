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

#ifndef STICK20SETUP_H
#define STICK20SETUP_H

#include <QDialog>
#include "device.h"

namespace Ui {
class Stick20Setup;
}

class Stick20Setup : public QDialog
{
    Q_OBJECT
    
public:
    explicit Stick20Setup(QWidget *parent = 0);
    ~Stick20Setup();
    
    Device *cryptostick;

private slots:
    void on_pushButton_Change_AdminPW_clicked();
    void on_pushButton_Ch_PW_clicked();
    void on_pushButton_Ch_Mat_APW_clicked();
    void on_pushButton_Ch_HiddenVol_clicked();


private:
    Ui::Stick20Setup *ui;
};

#endif // STICK20SETUP_H
