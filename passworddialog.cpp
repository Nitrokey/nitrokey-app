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


#include "device.h"
#include "mcvs-wrapper.h"

#include "passworddialog.h"
#include "ui_passworddialog.h"

#include "stick20matrixpassworddialog.h"
#include "cryptostick-applet.h"

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local defines

*******************************************************************************/

#define LOCAL_PASSWORD_SIZE         40              // Todo make define global

/*******************************************************************************

  PasswordDialog

  Constructor PasswordDialog

  Changes
  Date      Author        Info
  07.05.14  RB            Add matrix input

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

PasswordDialog::PasswordDialog(bool ShowMatrix,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);

    cryptostick = NULL;     // Set it manuel

    ui->checkBox_PasswordMatrix->setCheckState(Qt::Unchecked);

    if (FALSE == ShowMatrix)
    {
        ui->checkBox_PasswordMatrix->hide();
    }

    ui->lineEdit->setFocus();
}

/*******************************************************************************

  PasswordDialog

  Destructor PasswordDialog

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

PasswordDialog::~PasswordDialog()
{
    delete ui;
}

/*******************************************************************************

  init

  Changes
  Date      Author        Info
  04.02.14  RB            Function created
  07.07.14  RB              Retry counter added

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordDialog::init(char *text,int RetryCount)
{
    char text1[20];

    text1[0] = 0;
    if (-1 != RetryCount)
    {
        SNPRINTF (text1,sizeof (text1)," (Tries left: %d)",RetryCount);
    }
    ui->label->setText(tr(text)+tr(text1));
}



/*******************************************************************************

  getPassword

  Changes
  Date      Author        Info
  04.02.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordDialog::getPassword(char *text)
{
    if (FALSE == ui->checkBox_PasswordMatrix->isChecked())
    {
        STRCPY (&text[1],LOCAL_PASSWORD_SIZE-1,ui->lineEdit->text().toLatin1());
    }
    else
    {
        STRCPY (text,LOCAL_PASSWORD_SIZE,(char*)password);
    }
}


/*******************************************************************************

  on_checkBox_toggled

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void PasswordDialog::on_checkBox_toggled(bool checked)
{
    if (checked)
        ui->lineEdit->setEchoMode(QLineEdit::Normal);
    else
        ui->lineEdit->setEchoMode(QLineEdit::Password);
}

/*******************************************************************************

  on_checkBox_toggled

  Changes
  Date      Author        Info
  07.05.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordDialog::on_checkBox_PasswordMatrix_toggled(bool checked)
{

    if (checked)
    {
        ui->lineEdit->setDisabled(TRUE);
    }
    else
    {
        ui->lineEdit->setDisabled(FALSE);
    }

}

/*******************************************************************************

  on_buttonBox_accepted

  Changes
  Date      Author        Info
  07.05.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void PasswordDialog::on_buttonBox_accepted()
{
    int           n;
//    unsigned char password[50];
    QByteArray    passwordString;

    if (false == ui->checkBox_PasswordMatrix->isChecked())
    {
        // Send normal password
        password[0] = 'P';          // For normal password

        // Check the password length
        passwordString = ui->lineEdit->text().toLatin1();
        n = passwordString.size();
        if (30 <= n)
        {
            csApplet->warningBox("Your PIN is too long! Use not more than 30 characters.");
            done (FALSE);
            return;
        }
        if (6  > n)
        {
            csApplet->warningBox("Your PIN is too short. Use at least 6 characters.");
            done (FALSE);
            return;
        }

        if ((0 == strcmp (passwordString, "123456")) || (0 == strcmp (passwordString, "12345678")))
        {
            csApplet->warningBox("Warning: Default PIN is used.\nPlease change the PIN.");
        }
        memset (&password[1],0,49);
        memcpy(&password[1],passwordString.data(),n);
    }
    else
    {
        if (NULL != cryptostick)
        {
            // Get matrix password
            MatrixPasswordDialog dialog (this);

            dialog.setModal (TRUE);

            dialog.cryptostick        = cryptostick;
            dialog.PasswordLen        = 19;
            dialog.SetupInterfaceFlag = false;

            dialog.InitSecurePasswordDialog ();

            if (false == dialog.exec())
            {
                done (FALSE);
                return;
            }

            // Copy the matrix password
            password[0] = 'M';          // For matrix password
            dialog.CopyMatrixPassword((char*)&password[1],49);
        }
    }

}
