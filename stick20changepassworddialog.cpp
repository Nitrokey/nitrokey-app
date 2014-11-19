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

#include <QMessageBox>

#include "mcvs-wrapper.h"
#include "stick20changepassworddialog.h"
#include "ui_stick20changepassworddialog.h"
#include "stick20responsedialog.h"



/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local defines

*******************************************************************************/


/*******************************************************************************

  DialogChangePassword

  Constructor DialogChangePassword

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

DialogChangePassword::DialogChangePassword(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogChangePassword)
{
    ui->setupUi(this);

    ui->lineEdit_OldPW->setEchoMode(QLineEdit::Password);
    ui->lineEdit_OldPW->setMaxLength(20);
    ui->lineEdit_NewPW_1->setEchoMode(QLineEdit::Password);
    ui->lineEdit_NewPW_1->setMaxLength(20);
    ui->lineEdit_NewPW_2->setEchoMode(QLineEdit::Password);
    ui->lineEdit_NewPW_2->setMaxLength(20);


    ui->lineEdit_OldPW->setFocus();
}

/*******************************************************************************

  DialogChangePassword

  Destructor DialogChangePassword

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

DialogChangePassword::~DialogChangePassword()
{
    delete ui;
}

/*******************************************************************************

  InitData

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void DialogChangePassword::InitData(void)
{
    switch (PasswordKind)
    {
        case STICK20_PASSWORD_KIND_USER :
            ui->label->setText("Change user PIN");
            ui->label_2->setText("User PIN");
            ui->label_3->setText("New user PIN");
            ui->label_4->setText("New user PIN");
            break;
        case STICK20_PASSWORD_KIND_ADMIN :
            ui->label->setText("Change admin PIN");
            ui->label_2->setText("Admin PIN");
            ui->label_3->setText("New admin PIN");
            ui->label_4->setText("New admin PIN");
            break;
        case STICK20_PASSWORD_KIND_RESET_USER :
            ui->label->setText("Reset user PIN");
            ui->label_2->setText("Admin PIN");
            ui->label_3->setText("New user PIN");
            ui->label_4->setText("New user PIN");
            break;
    }
}

/*******************************************************************************

  CheckResponse

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

int DialogChangePassword::CheckResponse(bool NoStopFlag)
{
    Stick20ResponseTask ResponseTask(this,cryptostick,NULL);

    if (FALSE == NoStopFlag)
    {
        ResponseTask.NoStopWhenStatusOK ();
    }

    ResponseTask.GetResponse ();

    return (ResponseTask.ResultValue);
}

/*******************************************************************************

  SendNewPassword

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void DialogChangePassword::SendNewPassword(void)
{
    int ret;
    QByteArray PasswordString;

    unsigned char Data[STICK20_PASSOWRD_LEN+2];

// Set kind of password
    switch (PasswordKind)
    {
        case STICK20_PASSWORD_KIND_USER :
            Data[0] = 'P';
            break;
        case STICK20_PASSWORD_KIND_ADMIN :
            Data[0] = 'A';
            break;
        default :
            Data[0] = '?';
            break;
    }

// Send old password
    PasswordString = ui->lineEdit_OldPW->text().toLatin1();

    STRNCPY ((char*)&Data[1],STICK20_PASSOWRD_LEN-1,PasswordString.data(),STICK20_PASSOWRD_LEN);
    /*#ifdef _MSC_VER
    strncpy_s ((char*)&Data[1],STICK20_PASSOWRD_LEN-1,PasswordString.data(),STICK20_PASSOWRD_LEN);
    #else
    strncpy ((char*)&Data[1],PasswordString.data(),STICK20_PASSOWRD_LEN);
    #endif*/

    Data[STICK20_PASSOWRD_LEN+1] = 0;

    ret = cryptostick->stick20SendPassword (Data);

    if ((int)true == ret)
    {
        CheckResponse (TRUE);
    }
    else
    {
        // Todo
        return;
    }

// Change password
    PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();

    STRNCPY ((char*)&Data[1],STICK20_PASSOWRD_LEN,PasswordString.data(),STICK20_PASSOWRD_LEN);
    /*#ifdef _MSC_VER
    strncpy_s ((char*)&Data[1],STICK20_PASSOWRD_LEN,PasswordString.data(),STICK20_PASSOWRD_LEN);
    #else
    strncpy ((char*)&Data[1],PasswordString.data(),STICK20_PASSOWRD_LEN);
    #endif*/
    Data[STICK20_PASSOWRD_LEN+1] = 0;

    ret = cryptostick->stick20SendNewPassword (Data);

    if ((int)true == ret)
    {
        CheckResponse (FALSE);
    }
    else
    {
        // Todo
        return;
    }
}

/*******************************************************************************

  ResetUserPassword

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void DialogChangePassword::ResetUserPassword (void)
{
    int ret;
    QByteArray PasswordString;

    unsigned char Data[STICK20_PASSOWRD_LEN+2];

// Set kind of password
    Data[0] = 'A';

// Send old password
    PasswordString = ui->lineEdit_OldPW->text().toLatin1();

    STRNCPY ((char*)&Data[1],STICK20_PASSOWRD_LEN-1,PasswordString.data(),STICK20_PASSOWRD_LEN);
    /*#ifdef _MSC_VER
    strncpy_s ((char*)&Data[1],STICK20_PASSOWRD_LEN-1,PasswordString.data(),STICK20_PASSOWRD_LEN);
    #else
    strncpy ((char*)&Data[1],PasswordString.data(),STICK20_PASSOWRD_LEN);
    #endif*/
    Data[STICK20_PASSOWRD_LEN+1] = 0;

    ret = cryptostick->stick20SendPassword (Data);

    if ((int)true == ret)
    {
        CheckResponse (TRUE);
    }
    else
    {
        // Todo
        return;
    }

// Reset new user PIN
    PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();

    STRNCPY ((char*)&Data[1],STICK20_PASSOWRD_LEN-1,PasswordString.data(),STICK20_PASSOWRD_LEN);
    /*#ifdef _MSC_VER
    strncpy_s ((char*)&Data[1],STICK20_PASSOWRD_LEN-1,PasswordString.data(),STICK20_PASSOWRD_LEN);
    #else
    strncpy ((char*)&Data[1],PasswordString.data(),STICK20_PASSOWRD_LEN);
    #endif*/
    Data[STICK20_PASSOWRD_LEN+1] = 0;

    ret = cryptostick->unlockUserPassword (Data);

    if ((int)true == ret)
    {
        CheckResponse (FALSE);
    }
    else
    {
        // Todo
        return;
    }
}

/*******************************************************************************

  accept

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void DialogChangePassword::accept()
{
// Check the length of the old password
    if (6 > strlen (ui->lineEdit_OldPW->text().toLatin1()))
    {
        QMessageBox msgBox;
        QString OutputText;

        OutputText = "The minium length of the old password is " + QString("%1").arg(6)+ "chars";

        msgBox.setText(OutputText);
        msgBox.exec();
        return;
    }

// Check for correct new password entrys
    if (0 != strcmp (ui->lineEdit_NewPW_1->text().toLatin1(),ui->lineEdit_NewPW_2->text().toLatin1()))
    {
        QMessageBox msgBox;

        msgBox.setText("The new password entrys are not the same");
        msgBox.exec();
        return;
    }

// Check the new length of password
    if (STICK20_PASSOWRD_LEN < strlen (ui->lineEdit_NewPW_1->text().toLatin1()))
    {
        QMessageBox msgBox;
        QString OutputText;

        OutputText = "The maximum length of a password is " + QString("%1").arg(STICK20_PASSOWRD_LEN)+ "chars";

        msgBox.setText(OutputText);
        msgBox.exec();
        return;
    }

// Check the new length of password
    if (6 > strlen (ui->lineEdit_NewPW_1->text().toLatin1()))
    {
        QMessageBox msgBox;
        QString OutputText;

        OutputText = "The minium length of a password is " + QString("%1").arg(6)+ "chars";

        msgBox.setText(OutputText);
        msgBox.exec();
        return;
    }

// Send password to Stick 2.0
    switch (PasswordKind)
    {
        case STICK20_PASSWORD_KIND_USER :
        case STICK20_PASSWORD_KIND_ADMIN :
            SendNewPassword();
            break;
        case STICK20_PASSWORD_KIND_RESET_USER :
            ResetUserPassword ();
            break;
        default :
            break;
    }

    cryptostick->getStatus();

    done (true);
}

/*******************************************************************************

  on_checkBox_clicked

  Set echo mode for password edit

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/


void DialogChangePassword::on_checkBox_clicked(bool checked)
{
    if (checked)
    {
        ui->lineEdit_OldPW->setEchoMode(QLineEdit::Normal);
        ui->lineEdit_NewPW_1->setEchoMode(QLineEdit::Normal);
        ui->lineEdit_NewPW_2->setEchoMode(QLineEdit::Normal);
    }
    else
    {
        ui->lineEdit_OldPW->setEchoMode(QLineEdit::Password);
        ui->lineEdit_NewPW_1->setEchoMode(QLineEdit::Password);
        ui->lineEdit_NewPW_2->setEchoMode(QLineEdit::Password);
    }
}
