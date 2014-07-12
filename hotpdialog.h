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

#ifndef HOTPDIALOG_H
#define HOTPDIALOG_H

#include <QDialog>
#include <QTimer>
#include "device.h"

namespace Ui {
class HOTPDialog;
}

class HOTPDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit HOTPDialog(QWidget *parent);
    Device *device;
    uint8_t slotNumber;
    QString title;

    QTimer *TOTP_ValidTimer;
    uint64_t lastTOTPTime;
    uint8_t  lastInterval;

    ~HOTPDialog();

    void getNextCode();
    void setToHOTP();
    void setToTOTP();



private slots:
    void on_nextButton_clicked();

    void on_clipboardButton_clicked();

    void checkTOTP_Valid();

private:
    Ui::HOTPDialog *ui;


};

#endif // HOTPDIALOG_H
