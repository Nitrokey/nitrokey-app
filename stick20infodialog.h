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

#ifndef STICK20INFODIALOG_H
#define STICK20INFODIALOG_H

#include <QDialog>

namespace Ui {
class Stick20InfoDialog;
}

class Stick20InfoDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit Stick20InfoDialog(QWidget *parent = 0);
    ~Stick20InfoDialog();
    
private slots:
    void on_pushButton_clicked();

private:
    Ui::Stick20InfoDialog *ui;

    void showStick20Configuration (void);
};


#endif // STICK20INFODIALOG_H
