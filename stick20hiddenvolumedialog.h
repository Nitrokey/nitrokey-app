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

#ifndef STICK20HIDDENVOLUMEDIALOG_H
#define STICK20HIDDENVOLUMEDIALOG_H

#include <QDialog>
#include "device.h"

namespace Ui {
class stick20HiddenVolumeDialog;
}

class stick20HiddenVolumeDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit stick20HiddenVolumeDialog(QWidget *parent = 0);
    ~stick20HiddenVolumeDialog();

    HiddenVolumeSetup_tst HV_Setup_st;
    
private slots:
    void on_ShowPasswordCheckBox_toggled(bool checked);


    void on_buttonBox_accepted();

private:
    Ui::stick20HiddenVolumeDialog *ui;
};

#endif // STICK20HIDDENVOLUMEDIALOG_H
