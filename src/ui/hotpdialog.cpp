/*
 * Author: Copyright (C) Andrzej Surowiec 2012
 *
 *
 * This file is part of Nitrokey.
 *
 * Nitrokey is free software: you can redistribute it and/or modify
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

#include <QDateTime>
#include <QDebug>
#include <QClipboard>

#include "hotpdialog.h"
#include "ui_hotpdialog.h"

HOTPDialog::HOTPDialog (QWidget * parent):
QDialog (parent), ui (new Ui::HOTPDialog)
{
    device = NULL;
    slotNumber = 0;
    lastTOTPTime = 0;
    lastInterval = 0;
    lastClipboardTime = 0;
    TOTP_ValidTimer = new QTimer (this);

    // Start timer for polling stick response
    connect (TOTP_ValidTimer, SIGNAL (timeout ()), this, SLOT (checkTOTP_Valid ()));
    TOTP_ValidTimer->start (100);

    Clipboard_ValidTimer = new QTimer (this);

    // Start timer for polling stick response
    connect (Clipboard_ValidTimer, SIGNAL (timeout ()), this, SLOT (checkClipboard_Valid ()));
    Clipboard_ValidTimer->start (100);

    clipboard = QApplication::clipboard ();
    ui->setupUi (this);
}

void HOTPDialog::getNextCode ()
{
uint8_t result[18];

    memset (result, 0, 18);
uint32_t code;

uint8_t config;

    lastInterval = ui->intervalSpinBox->value ();

    if (lastInterval < 1)
        lastInterval = 1;

    if (slotNumber >= 0x20)
        device->TOTPSlots[slotNumber - 0x20]->interval = lastInterval;

QString output;

    lastTOTPTime = QDateTime::currentDateTime ().toTime_t ();

    device->getCode (slotNumber, lastTOTPTime / lastInterval, lastTOTPTime, lastInterval, result);

    // cryptostick->getCode(slotNo,1,result);
    code = result[0] + (result[1] << 8) + (result[2] << 16) + (result[3] << 24);
    config = result[4];



    /* if (config&(1<<2)) output.append(QByteArray((char *)(result+5),12)); */

    if (config & (1 << 0))
    {
        code = code % 100000000;
        output.append (QString ("%1").arg (QString::number (code), 8, '0'));
    }
    else
    {

        code = code % 1000000;
        output.append (QString ("%1").arg (QString::number (code), 6, '0'));
    }


    qDebug () << "Current time:" << lastTOTPTime;
    qDebug () << "Counter:" << lastTOTPTime / lastInterval;
    qDebug () << "TOTP:" << code;

    ui->lineEdit->setText (output);
    copyToClipboard (ui->lineEdit->text ());

}

void HOTPDialog::setToHOTP ()
{
    ui->label->setText (tr ("Your HOTP:"));
    ui->nextButton->setText (tr ("Next HOTP"));
    ui->labelNotify->setText (tr ("HOTP copied to clipboard"));
    this->setWindowTitle (title);
    ui->intervalLabel->hide ();
    ui->intervalSpinBox->hide ();
    ui->validTimer->hide ();

}

void HOTPDialog::setToTOTP ()
{
    ui->label->setText (tr ("Your TOTP:"));
    ui->nextButton->setText (tr ("Generate TOTP"));
    this->setWindowTitle (title);
    ui->intervalLabel->show ();
    ui->labelNotify->setText (tr ("TOTP copied to clipboard"));

    ui->intervalSpinBox->setValue (device->TOTPSlots[slotNumber - 0x20]->interval);
    ui->intervalSpinBox->show ();
    ui->validTimer->show ();

}

HOTPDialog::~HOTPDialog ()
{
    // Kill timer
    TOTP_ValidTimer->stop ();
delete TOTP_ValidTimer;

delete ui;
}

void HOTPDialog::on_nextButton_clicked ()
{

    getNextCode ();


}

void HOTPDialog::copyToClipboard (QString text)
{
    lastClipboardTime = QDateTime::currentDateTime ().toTime_t ();
    clipboard->setText (text);
    ui->labelNotify->show ();
    // this->accept();
}


/*******************************************************************************

  checkTOTP_Valid

  Changes
  Date      Author        Info
  17.03.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
#define TOTP_MORE_THAN_5_SEC_TO_INVALID 0
#define TOTP_LESS_THAN_5_SEC_TO_INVALID 1
#define TOTP_IS_INVALID                 2


void HOTPDialog::checkTOTP_Valid ()
{
uint64_t currentTime;

uint64_t checkTime;

uint8_t state;

QPalette palette;

    state = TOTP_IS_INVALID;

    currentTime = QDateTime::currentDateTime ().toTime_t ();

    checkTime = (lastTOTPTime / (uint64_t) lastInterval) * (uint64_t) lastInterval;
    if (checkTime + (uint64_t) lastInterval - (uint64_t) 5 > currentTime)
    {
        state = TOTP_MORE_THAN_5_SEC_TO_INVALID;
    }

    if ((checkTime + (uint64_t) lastInterval - (uint64_t) 5 <= currentTime) && (checkTime + lastInterval > currentTime))
    {
        state = TOTP_LESS_THAN_5_SEC_TO_INVALID;
    }

    palette = ui->validTimer->palette ();
    ui->validTimer->setAutoFillBackground (true);
    switch (state)
    {
        case TOTP_MORE_THAN_5_SEC_TO_INVALID:
            ui->validTimer->setText (tr ("Valid"));
            palette.setColor (ui->validTimer->backgroundRole (), Qt::green);
            break;
        case TOTP_LESS_THAN_5_SEC_TO_INVALID:
            ui->validTimer->setText (tr ("Valid"));
            palette.setColor (ui->validTimer->backgroundRole (), Qt::yellow);
            break;
        case TOTP_IS_INVALID:
            ui->validTimer->setText (tr ("Invalid"));
            palette.setColor (ui->validTimer->backgroundRole (), Qt::red);
            break;
    }

    ui->validTimer->setPalette (palette);

}

/*******************************************************************************

  checkClipboard_Valid

  Changes
  Date      Author        Info
  12.07.14  SN            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void HOTPDialog::checkClipboard_Valid ()
{
uint64_t currentTime;

    // uint64_t checkTime;

    currentTime = QDateTime::currentDateTime ().toTime_t ();

    if (currentTime >= lastClipboardTime + (uint64_t) 60 && (QString::compare (clipboard->text (), ui->lineEdit->text ()) == 0))
    {
        copyToClipboard (QString (""));
        ui->labelNotify->hide ();
    }
    if (QString::compare (clipboard->text (), ui->lineEdit->text ()) != 0)
    {
        ui->labelNotify->hide ();
    }

}
