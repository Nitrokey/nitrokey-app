/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
*
* This file is part of Nitrokey 2
*
* Nitrokey 2  is free software: you can redistribute it and/or modify
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

#include "mcvs-wrapper.h"
#include "stick20changepassworddialog.h"
#include "ui_stick20changepassworddialog.h"
#include "stick20responsedialog.h"
#include "nitrokey-applet.h"


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
    cryptostick = NULL;
    PasswordKind = 0;
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
        case STICK10_PASSWORD_KIND_USER :
            ui->label_2->setText("Old user PIN");
            ui->label_3->setText("New user PIN");
            ui->label_4->setText("New user PIN");
            break;
        case STICK20_PASSWORD_KIND_ADMIN :
        case STICK10_PASSWORD_KIND_ADMIN :
            ui->label_2->setText("Admin PIN");
            ui->label_3->setText("New admin PIN");
            ui->label_4->setText("New admin PIN");
            break;
        case STICK20_PASSWORD_KIND_RESET_USER :
        case STICK10_PASSWORD_KIND_RESET_USER :
            this->setWindowTitle("Reset user PIN");
            ui->label_2->setText("Admin PIN:");
            ui->label_3->setText("New user PIN:");
            ui->label_4->setText("New user PIN:");
            break;
        case STICK20_PASSWORD_KIND_UPDATE:
            this->setWindowTitle("Change Update PIN");
            ui->label_2->setText("Update PIN:");
            ui->label_3->setText("New Update PIN:");
            ui->label_4->setText("New Update PIN (confirm):");
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
    int password_length;
    QByteArray PasswordString;

    password_length = STICK20_PASSOWRD_LEN;
    unsigned char Data[password_length + 2];

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

  Stick10ChangePassword

  Reviews
  Date      Reviewer        Info
  21.10.13  GG              Created function

*******************************************************************************/

void DialogChangePassword::Stick10ChangePassword(void)
{
    int ret;
    int password_length;
    QByteArray PasswordString;

    password_length = STICK10_PASSWORD_LEN;
    unsigned char old_pin[password_length + 1];
    unsigned char new_pin[password_length + 1];
    memset(old_pin, 0, password_length + 1);
    memset(new_pin, 0, password_length + 1);

    PasswordString = ui->lineEdit_OldPW->text().toLatin1();
    strncpy ( (char*)old_pin, PasswordString.data(), password_length);

    PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();
    strncpy ( (char*)new_pin, PasswordString.data(), password_length);

    // Change password
    if ( PasswordKind == STICK10_PASSWORD_KIND_ADMIN )
        ret = cryptostick->changeAdminPin (old_pin, new_pin);
    else //if ( PasswordKind == STICK10_PASSWORD_KIND_USER )
        ret = cryptostick->changeUserPin (old_pin, new_pin);

    if (ret == CMD_STATUS_WRONG_PASSWORD) {
        csApplet->warningBox("Wrong password.");
    } else if (ret != CMD_STATUS_OK) {
        csApplet->warningBox(tr("Couldn't change %1 password").arg((PasswordKind == STICK10_PASSWORD_KIND_USER)?"user":"admin"));
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


void DialogChangePassword::ResetUserPasswordStick10 (void)
{
    int ret;
    QByteArray PasswordString;

    unsigned char data[50+1];
    memset(data, 0, 51);

    // New User PIN
    PasswordString = ui->lineEdit_OldPW->text().toLatin1();
    STRNCPY ((char*)data, 25, PasswordString.data(), 25);

    // Admin PIN
    PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();
    STRNCPY ((char*)&(data[25]), 25, PasswordString.data(), 25);

    ret = cryptostick->unlockUserPasswordStick10 (data);

    if ( CMD_STATUS_OK != ret)
    {
        if (CMD_STATUS_WRONG_PASSWORD == ret)
        {
            csApplet->warningBox("Wrong Admin PIN.");
        }
        else
        {
            csApplet->warningBox(tr("Couldn't unblock the user PIN. Error: %1").arg(ret));
        }
    } else {
        csApplet->messageBox("User PIN successfully unblocked");
    }
}

void DialogChangePassword::Stick20ChangeUpdatePassword(void)
{
    int ret;
    int password_length;
    QByteArray PasswordString;

    password_length = CS20_MAX_UPDATE_PASSWORD_LEN;
    unsigned char old_pin[password_length + 1];
    unsigned char new_pin[password_length + 1];
    memset(old_pin, 0, password_length + 1);
    memset(new_pin, 0, password_length + 1);

    PasswordString = ui->lineEdit_OldPW->text().toLatin1();
    strncpy ( (char*)old_pin, PasswordString.data(), password_length);

    PasswordString = ui->lineEdit_NewPW_1->text().toLatin1();
    strncpy ( (char*)new_pin, PasswordString.data(), password_length);

    // Change password
    ret = cryptostick->stick20NewUpdatePassword ((uint8_t *)old_pin,(uint8_t *)new_pin);

    if (!ret)
        csApplet->warningBox("Wrong password.");
}

/*******************************************************************************

  accept

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*
*******************************************************************************/

void DialogChangePassword::accept()
{
// Check the length of the old password
    if (6 > strlen (ui->lineEdit_OldPW->text().toLatin1()))
    {
        clearFields();
        QString OutputText;

        OutputText = "The minimum length of the old password is " + QString("%1").arg(6)+ "chars";
        
        csApplet->warningBox(OutputText);
        return;
    }

// Check for correct new password entrys
    if (0 != strcmp (ui->lineEdit_NewPW_1->text().toLatin1(),ui->lineEdit_NewPW_2->text().toLatin1()))
    {
        clearFields();
        csApplet->warningBox("The new password entrys are not the same");
        return;
    }

// Check the new length of password
    if (STICK20_PASSOWRD_LEN < strlen (ui->lineEdit_NewPW_1->text().toLatin1()))
    {
        clearFields();
        QString OutputText;

        OutputText = "The maximum length of a password is " + QString("%1").arg(STICK20_PASSOWRD_LEN)+ "chars";

        csApplet->warningBox(OutputText);
        return;
    }

// Check the new length of password
    if (6 > strlen (ui->lineEdit_NewPW_1->text().toLatin1()))
    {
        clearFields();
        QString OutputText;

        OutputText = "The minimum length of a password is " + QString("%1").arg(6)+ "chars";

        csApplet->warningBox(OutputText);
        return;
    }

// Send password to Stick 2.0
    switch (PasswordKind)
    {
        case STICK20_PASSWORD_KIND_USER :
        case STICK20_PASSWORD_KIND_ADMIN :
            SendNewPassword();
            break;
        case STICK10_PASSWORD_KIND_USER :
        case STICK10_PASSWORD_KIND_ADMIN :
            Stick10ChangePassword();
            break;
        case STICK20_PASSWORD_KIND_RESET_USER :
            ResetUserPassword ();
            break;
        case STICK10_PASSWORD_KIND_RESET_USER :
            ResetUserPasswordStick10 ();
            break;
        case STICK20_PASSWORD_KIND_UPDATE:
            Stick20ChangeUpdatePassword();
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

void DialogChangePassword::clearFields()
{
    ui->lineEdit_OldPW->clear();
    ui->lineEdit_NewPW_1->clear();
    ui->lineEdit_NewPW_2->clear();

    ui->lineEdit_OldPW->setFocus();
}
