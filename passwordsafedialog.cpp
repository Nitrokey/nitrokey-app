/*
* Author: Copyright (C)  Rudolf Boeddeker  Date: 2014-08-04
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

#include <QClipboard>

#include "cryptostick-applet.h"
#include "passwordsafedialog.h"
#include "ui_passwordsafedialog.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

class OwnSleep : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep (unsigned long secs) {QThread::sleep(secs);}
};


PasswordSafeDialog::PasswordSafeDialog(int Slot,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordSafeDialog)
{
    QString MsgText;
    char Text[100];

    ui->setupUi(this);


    clipboard = QApplication::clipboard();

    delaySendTextInMs    = 1000;      // = 1.0 sec
    UsedSlot             = Slot;

    ui->spinBoxDelay->setMinimum(500);
    ui->spinBoxDelay->setMaximum(10000);
    ui->spinBoxDelay->setSingleStep (500);

    delaySendTextInMs    = 1000;      // = 1.0 sec
    ui->spinBoxDelay->setValue(delaySendTextInMs);

    ui->radioCutUPaste->setChecked(true);
    ui->radioKeyboard->setChecked(false);
    ui->spinBoxDelay->setEnabled(false);

    MsgText.append("Password Safe Slot ");
    MsgText.append(QString::number(UsedSlot+1,10));
/*
    MsgText.append(" -");

    strcpy (Text,(char*)&cryptostick->passwordSafeSlotNames[UsedSlot][0]);


    MsgText.append((char*)cryptostick->passwordSafeSlotNames[UsedSlot]);
*/
    MsgText.append(" clicked.\nPress <Button> to copy value to clipboard");

    ui->labelInfo->setText(MsgText);

}

PasswordSafeDialog::~PasswordSafeDialog()
{
    delete ui;
}

/*******************************************************************************

  on_ButtonSendpassword_clicked

  Changes
  Date      Author        Info
  04.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordSafeDialog::on_ButtonSendpassword_clicked()
{
    QString MsgText;
    int     ret_s32;

    if (true == ui->radioKeyboard->isChecked())
    {
        OwnSleep::msleep(delaySendTextInMs);

        ret_s32 = cryptostick->passwordSafeSendSlotDataViaHID (UsedSlot,PWS_SEND_PASSWORD);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't send password chars via HID");
            return;
        }
    }

    if (true == ui->radioCutUPaste->isChecked())
    {
        ret_s32 = cryptostick->getPasswordSafeSlotPassword(UsedSlot);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't get password");
            return;
        }

        clipboard->setText((char*)cryptostick->passwordSafePassword);
    }
}

/*******************************************************************************

  on_ButtonSendPW_LN_clicked

  Changes
  Date      Author        Info
  04.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordSafeDialog::on_ButtonSendPW_LN_clicked()
{
    QString MsgText;
    int     ret_s32;

    if (true == ui->radioKeyboard->isChecked())
    {
        OwnSleep::msleep(delaySendTextInMs);

        ret_s32 = cryptostick->passwordSafeSendSlotDataViaHID (UsedSlot,PWS_SEND_LOGINNAME);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't send loginname via keyboard");
            return;
        }

        ret_s32 = cryptostick->passwordSafeSendSlotDataViaHID (UsedSlot,PWS_SEND_TAB);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't send CR via keyboard");
            return;
        }

        ret_s32 = cryptostick->passwordSafeSendSlotDataViaHID (UsedSlot,PWS_SEND_PASSWORD);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't send password via keyboard");
            return;
        }

    }

    if (true == ui->radioCutUPaste->isChecked())
    {
        ret_s32 = cryptostick->getPasswordSafeSlotLoginName(UsedSlot);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't get password");
            return;
        }

        MsgText.append((char*)cryptostick->passwordSafeLoginName);
        MsgText.append((char)9);

        ret_s32 = cryptostick->getPasswordSafeSlotPassword(UsedSlot);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't get password");
            return;
        }
        MsgText.append((char*)cryptostick->passwordSafePassword);

        clipboard->setText(MsgText);
    }
}
/*******************************************************************************

  on_ButtonSendLoginname_clicked

  Changes
  Date      Author        Info
  04.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordSafeDialog::on_ButtonSendLoginname_clicked()
{
    QString MsgText;
    int     ret_s32;

    if (true == ui->radioKeyboard->isChecked())
    {
        OwnSleep::msleep(delaySendTextInMs);

        ret_s32 = cryptostick->passwordSafeSendSlotDataViaHID (UsedSlot,PWS_SEND_LOGINNAME);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't send loginname chars via HID");
            return;
        }
    }

    if (true == ui->radioCutUPaste->isChecked())
    {
        ret_s32 = cryptostick->getPasswordSafeSlotLoginName(UsedSlot);
        if (ERR_NO_ERROR != ret_s32)
        {
            csApplet->warningBox("Can't get password");
            return;
        }

        clipboard->setText((char*)cryptostick->passwordSafeLoginName);
    }
}

/*******************************************************************************

  on_ButtonOk_clicked

  Changes
  Date      Author        Info
  04.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordSafeDialog::on_ButtonOk_clicked()
{
    done(TRUE);
}

/*******************************************************************************

  on_ButtonOk_clicked

  Changes
  Date      Author        Info
  04.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordSafeDialog::on_spinBoxDelay_valueChanged(int arg1)
{
    QString MsgText;

    delaySendTextInMs = ui->spinBoxDelay->value();

    if (true == ui->radioKeyboard->isChecked())
    {
        MsgText.append("PW Safe Slot ");
        MsgText.append(QString::number(UsedSlot+1,10));
        MsgText.append(" clicked.\nPress <Button> and set cursor to the input dialog.\nThe value is send in ");
        MsgText.append(QString::number((float)delaySendTextInMs/1000.0,'f',1));
        MsgText.append(" seconds");

        ui->labelInfo->setText(MsgText);
    }
}

/*******************************************************************************

  on_radioCutUPaste_clicked

  Changes
  Date      Author        Info
  04.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordSafeDialog::on_radioCutUPaste_clicked()
{
    QString MsgText;

    ui->radioCutUPaste->setChecked(true);
    ui->radioKeyboard->setChecked(false);

    ui->spinBoxDelay->setEnabled(false);

    MsgText.append("PW Safe Slot ");
    MsgText.append(QString::number(UsedSlot+1,10));
    MsgText.append(" clicked.\nPress <Button> to copy value to clipboard");

    ui->labelInfo->setText(MsgText);

}

/*******************************************************************************

  on_radioKeyboard_clicked

  Changes
  Date      Author        Info
  04.08.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


void PasswordSafeDialog::on_radioKeyboard_clicked()
{
    QString MsgText;

    ui->radioCutUPaste->setChecked(false);
    ui->radioKeyboard->setChecked(true);

    ui->spinBoxDelay->setEnabled(true);

    MsgText.append("PW Safe Slot ");
    MsgText.append(QString::number(UsedSlot+1,10));
    MsgText.append(" clicked.\nPress <Button> and set cursor to the input dialog.\nThe value is send in ");
    MsgText.append(QString::number((float)delaySendTextInMs/1000.0,'f',1));
    MsgText.append(" seconds");

    ui->labelInfo->setText(MsgText);

}
