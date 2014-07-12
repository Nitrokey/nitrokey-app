/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2014-04-13
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

#include "device.h"
#include "stick20hiddenvolumedialog.h"
#include "ui_stick20hiddenvolumedialog.h"


stick20HiddenVolumeDialog::stick20HiddenVolumeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::stick20HiddenVolumeDialog)
{
    ui->setupUi(this);

    HV_Setup_st.SlotNr_u8                   =   0;
    HV_Setup_st.StartBlockPercent_u8        =  80;
    HV_Setup_st.EndBlockPercent_u8          = 100;
    HV_Setup_st.HiddenVolumePassword_au8[0] =   0;

    ui->comboBox->setCurrentIndex(HV_Setup_st.SlotNr_u8);

    ui->StartBlockSpin->setMaximum(99);
    ui->StartBlockSpin->setMinimum(20);
    ui->StartBlockSpin->setValue(HV_Setup_st.StartBlockPercent_u8);

    ui->EndBlockSpin->setMaximum(100);
    ui->EndBlockSpin->setMinimum(21);
    ui->EndBlockSpin->setValue(HV_Setup_st.EndBlockPercent_u8);

    ui->HVPasswordEdit->setMaxLength(MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
    ui->HVPasswordEdit->setText(QString((char*)HV_Setup_st.HiddenVolumePassword_au8));
}

stick20HiddenVolumeDialog::~stick20HiddenVolumeDialog()
{
    delete ui;
}


void stick20HiddenVolumeDialog::on_ShowPasswordCheckBox_toggled(bool checked)
{
    if (checked)
        ui->HVPasswordEdit->setEchoMode(QLineEdit::Normal);
    else
        ui->HVPasswordEdit->setEchoMode(QLineEdit::Password);
}



void stick20HiddenVolumeDialog::on_buttonBox_accepted()
{
    HV_Setup_st.SlotNr_u8            = ui->comboBox->currentIndex();
    HV_Setup_st.StartBlockPercent_u8 = ui->StartBlockSpin->value();
    HV_Setup_st.EndBlockPercent_u8   = ui->EndBlockSpin->value();

    strncpy ((char*)HV_Setup_st.HiddenVolumePassword_au8,ui->HVPasswordEdit->text().toLatin1(),MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
    HV_Setup_st.HiddenVolumePassword_au8[MAX_HIDDEN_VOLUME_PASSOWORD_SIZE] = 0;
}
