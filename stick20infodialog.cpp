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

#include "stick20infodialog.h"
#include "ui_stick20infodialog.h"

#include "stick20hid.h"


Stick20InfoDialog::Stick20InfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Stick20InfoDialog)
{
    ui->setupUi(this);
    showStick20Configuration ();
}

Stick20InfoDialog::~Stick20InfoDialog()
{
    delete ui;
}


/*******************************************************************************

  showStick20Configuration

  Changes
  Date      Author        Info
  17.04.14  RB            Function created

  Reviews
  Date      Reviewer        Info

*******************************************************************************/


void Stick20InfoDialog::showStick20Configuration (void)
{
    QString OutputText;

    OutputText.append(QString("Crypto Stick Storage status\n\n"));

    if (TRUE == HID_Stick20Configuration_st.StickKeysNotInitiated)
    {
        OutputText.append(QString(" ***  Warning: Device is not secure  ***")).append("\n");
        OutputText.append(QString(" **  Select -Init encrypted volumes- **")).append("\n").append("\n");
    }

    OutputText.append(QString("Firmware version     "));
    OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.VersionInfo_au8[0])));
    OutputText.append(QString("."));
    OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.VersionInfo_au8[1])));
    OutputText.append(QString("\n"));

    if (TRUE == HID_Stick20Configuration_st.FirmwareLocked_u8)
    {
        OutputText.append(QString("*** Firmware is locked *** ")).append("\n");
    }

    if (READ_WRITE_ACTIVE == HID_Stick20Configuration_st.ReadWriteFlagUncryptedVolume_u8)
    {
        OutputText.append(QString("Unencrypted volume   READ/WRITE mode ")).append("\n");
    }
    else
    {
        OutputText.append(QString("Unencrypted volume   READ ONLY mode ")).append("\n");
    }

    if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_CRYPTED_VOLUME_BIT_PLACE)))
    {
        OutputText.append(QString("Encrypted volume     active")).append("\n");
    }
    else
    {
        OutputText.append(QString("Encrypted volume     not active")).append("\n");
    }

    if (0 != (HID_Stick20Configuration_st.VolumeActiceFlag_u8 & (1 << SD_HIDDEN_VOLUME_BIT_PLACE)))
    {
        OutputText.append(QString("Hidden volume        active")).append("\n");
    }



    OutputText.append(QString("SD change counter    "));
    OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.NewSDCardFound_u8 >> 1))).append("\n");

    OutputText.append(QString("SD erase counter     "));
    OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.SDFillWithRandomChars_u8 >> 1))).append("\n");


    OutputText.append(QString("\n"));
    OutputText.append(QString("SD card info\n"));

    OutputText.append(QString(" ID     0x"));
    OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.ActiveSD_CardID_u32,16))).append("\n");


    if (0 != (HID_Stick20Configuration_st.NewSDCardFound_u8 & 0x01))
    {
//        OutputText.append(QString(" *** New SD card found ***\n"));
    }

    if (0 == (HID_Stick20Configuration_st.SDFillWithRandomChars_u8 & 0x01))
    {
        OutputText.append(QString(" *** Storage is not initialized with random data ***\n"));
    }


    OutputText.append(QString("\n"));
    OutputText.append(QString("Smartcard info\n"));

    OutputText.append(QString(" ID     0x"));
    OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.ActiveSmartCardID_u32,16))).append("\n");

    OutputText.append(QString(" Password retry counter\n"));
    OutputText.append(QString("  Admin : "));
    OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.AdminPwRetryCount))).append("\n");

    OutputText.append(QString("  User  : "));
    OutputText.append(QString("%1").arg(QString::number(HID_Stick20Configuration_st.UserPwRetryCount))).append("\n");

    ui->Infotext->setText(OutputText);


}

void Stick20InfoDialog::on_pushButton_clicked()
{
    done(true);
}
