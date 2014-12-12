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

#ifndef PINDIALOG_H
#define PINDIALOG_H

#include <QDialog>

#include "device.h"
#include "ui_pindialog.h"

namespace Ui {
class PinDialog;
}

class PinDialog : public QDialog
{
    Q_OBJECT
    
public:
    enum Usage {PLAIN, PREFIXED};
    enum PinType {USER_PIN, ADMIN_PIN, OTHER};

    unsigned char password[50];
    Device *cryptostick;

    explicit PinDialog( const QString & title, const QString & label,
                        Device *cryptostick, Usage usage, PinType pinType,
                        bool ShowMatrix=FALSE, QWidget *parent = NULL);
    ~PinDialog();

//    void init(char *text,int RetryCount);
    void getPassword(char *text);
    void getPassword(QString &pin);

private slots:
    void on_checkBox_toggled(bool checked);

    void on_checkBox_PasswordMatrix_toggled(bool checked);

    void onOkButtonClicked();

private:
    Ui::PinDialog *ui;

    Usage _usage;
    PinType _pinType;

    void updateTryCounter();
    void clearBuffers();
};

#endif // PINDIALOG_H
