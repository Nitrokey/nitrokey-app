/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2014-04-13
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
#include "math.h"
#include "device.h"
#include "stick20hiddenvolumedialog.h"
#include "ui_stick20hiddenvolumedialog.h"
#include "nitrokey-applet.h"


stick20HiddenVolumeDialog::stick20HiddenVolumeDialog (QWidget * parent):
QDialog (parent), ui (new Ui::stick20HiddenVolumeDialog)
{
    ui->setupUi (this);

    SdCardHighWatermark_Read_Min = 0;
    SdCardHighWatermark_Read_Max = 0;
    SdCardHighWatermark_Write_Min = 0;
    SdCardHighWatermark_Write_Max = 0;

    HV_Setup_st.SlotNr_u8 = 0;
    HV_Setup_st.StartBlockPercent_u8 = 70;
    HV_Setup_st.EndBlockPercent_u8 = 90;
    HV_Setup_st.HiddenVolumePassword_au8[0] = 0;

    ui->comboBox->setCurrentIndex (HV_Setup_st.SlotNr_u8);

    ui->StartBlockSpin->setMaximum (89);
    ui->StartBlockSpin->setMinimum (10);
    ui->StartBlockSpin->setValue (HV_Setup_st.StartBlockPercent_u8);

    ui->EndBlockSpin->setMaximum (90);
    ui->EndBlockSpin->setMinimum (11);
    ui->EndBlockSpin->setValue (HV_Setup_st.EndBlockPercent_u8);

    ui->HVPasswordEdit->setMaxLength (MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
    ui->HVPasswordEdit->
        setText (QString ((char *) HV_Setup_st.HiddenVolumePassword_au8));

    ui->HVPasswordEdit_2->setMaxLength (MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
    ui->HVPasswordEdit_2->
        setText (QString ((char *) HV_Setup_st.HiddenVolumePassword_au8));

    ui->HVPasswordEdit->setFocus ();

    on_HVPasswordEdit_textChanged ("");
}

stick20HiddenVolumeDialog::~stick20HiddenVolumeDialog ()
{
delete ui;
}


void stick20HiddenVolumeDialog::on_ShowPasswordCheckBox_toggled (bool checked)
{
    if (checked)
    {
        ui->HVPasswordEdit_2->setEchoMode (QLineEdit::Normal);
        ui->HVPasswordEdit->setEchoMode (QLineEdit::Normal);
    }
    else
    {
        ui->HVPasswordEdit_2->setEchoMode (QLineEdit::Password);
        ui->HVPasswordEdit->setEchoMode (QLineEdit::Password);
    }
}

void stick20HiddenVolumeDialog::on_buttonBox_clicked (QAbstractButton *
                                                      button)
{
    if (button == ui->buttonBox->button (QDialogButtonBox::Ok))
    {
        if (8 > strlen (ui->HVPasswordEdit->text ().toLatin1 ().data ()))
        {
            csApplet->
                warningBox (tr
                            ("Your password is too short. Use at least 8 characters."));
            return;
        }

        if (ui->HVPasswordEdit->text ().toLatin1 () !=
            ui->HVPasswordEdit_2->text ().toLatin1 ())
        {
            csApplet->warningBox (tr ("The passwords are not identical"));
            return;
        }

        HV_Setup_st.SlotNr_u8 = ui->comboBox->currentIndex ();
        HV_Setup_st.StartBlockPercent_u8 = ui->StartBlockSpin->value ();
        HV_Setup_st.EndBlockPercent_u8 = ui->EndBlockSpin->value ();

        if (HV_Setup_st.StartBlockPercent_u8 >=
            HV_Setup_st.EndBlockPercent_u8)
        {
            csApplet->warningBox (tr ("Wrong size of hidden volume"));
            return;
        }

        STRNCPY ((char *) HV_Setup_st.HiddenVolumePassword_au8,
                 sizeof (HV_Setup_st.HiddenVolumePassword_au8),
                 ui->HVPasswordEdit->text ().toLatin1 (),
                 MAX_HIDDEN_VOLUME_PASSOWORD_SIZE);
        HV_Setup_st.
            HiddenVolumePassword_au8[MAX_HIDDEN_VOLUME_PASSOWORD_SIZE] = 0;
        done (true);
    }

    if (button == ui->buttonBox->button (QDialogButtonBox::Cancel))
    {
        done (false);
    }
}

// Based from
// http://www.emoticode.net/c/optimized-f-the-shannf-the-shannon-entropy-algorithm.html
// */



int stick20HiddenVolumeDialog::GetCharsetSpace (unsigned char* Password,
                                                size_t size)
{
    int i;

    int HasLowerAlpha;

    int HasUpperAlpha;

    int HasGermanChars;

    int HasNumber;

    int HasSpecialChars1;

    int HasSpecialChars2;

    int HasSpecialChars3;

    int CharSpace;

    HasLowerAlpha = FALSE;
    HasUpperAlpha = FALSE;
    HasGermanChars = FALSE;
    HasNumber = FALSE;
    HasSpecialChars1 = FALSE;
    HasSpecialChars2 = FALSE;
    HasSpecialChars3 = FALSE;
    CharSpace = 0;

    for (i = 0; i < (int) size; i++)
    {
        if ((FALSE == HasLowerAlpha)
            && (0 != strchr ("abcdefghijklmnopqrstuvwxyz", Password[i])))
        {
            CharSpace += 26;
            HasLowerAlpha = TRUE;
        }
        if ((FALSE == HasUpperAlpha)
            && (0 != strchr ("ABCDEFGHIJKLMNOPQRSTUVWXYZ", Password[i])))
        {
            CharSpace += 26;
            HasUpperAlpha = TRUE;
        }
        if ((FALSE == HasGermanChars)
            && (0 != strchr ("öäüÖÄÜß", Password[i])))
        {
            CharSpace += 7;
            HasGermanChars = TRUE;
        }
        if ((FALSE == HasNumber) && (0 != strchr ("0123456789", Password[i])))
        {
            CharSpace += 10;
            HasNumber = TRUE;
        }
        if ((FALSE == HasSpecialChars1)
            && (0 != strchr ("!\"§$%&/()=?'", Password[i])))
        {
            CharSpace += 11;
            HasSpecialChars1 = TRUE;
        }
        if ((FALSE == HasSpecialChars2)
            && (0 != strchr ("',.-#+´;:_*<>^°`", Password[i])))
        {
            CharSpace += 16;
            HasSpecialChars2 = TRUE;
        }
        if ((FALSE == HasSpecialChars3)
            && (0 != strchr ("~[{]}\\|€@", Password[i])))
        {
            CharSpace += 9;
            HasSpecialChars3 = TRUE;
        }
    }

    return (CharSpace);
}


double stick20HiddenVolumeDialog::GetEntropy (unsigned char* Password,
                                              size_t size)
{
    double Entropy = 0.0;

    int CharsetSpace;

    CharsetSpace = GetCharsetSpace (Password, size);

    Entropy = (double) size* (log ((double) CharsetSpace) / log (2.0)); // Entropy
                                                                        // by
                                                                        // CharsetSpace
                                                                        // *
                                                                        // size

    return (Entropy);
}


void stick20HiddenVolumeDialog::
on_HVPasswordEdit_textChanged (const QString & arg1)
{
    int Len;

    double Entropy;

    Len = arg1.length ();
    Entropy = GetEntropy ((unsigned char *) arg1.toLatin1 ().data (), Len);

    if (0 < Entropy)
    {
        // ui->HVEntropieLabel->setText(QString ("%1").sprintf("Entropy
        // guess: %3.1lf bits for random chars\nEntropy guess: %3.1lf for
        // real words",Entropy,Entropy/2.0));
        ui->HVEntropieRealWords->setText (QString ("%1").
                                          sprintf (" %3.1lf for real words",
                                                   Entropy / 2.0));
        ui->HVEntropieRandChars->setText (QString ("%1").
                                          sprintf
                                          (" %3.1lf bits for random chars",
                                           Entropy));
    }
    else
    {
        // ui->HVEntropieLabel->setText(QString ("%1").sprintf("Entropy
        // guess: %3.1lf bits for random chars\nEntropy guess: %3.1lf for
        // real words",0.0,0.0));
        ui->HVEntropieRealWords->setText (QString ("%1").
                                          sprintf (" %3.1lf for real words",
                                                   0.0));
        ui->HVEntropieRandChars->setText (QString ("%1").
                                          sprintf
                                          (" %3.1lf bits for random chars",
                                           0.0));
    }
}

void stick20HiddenVolumeDialog::setHighWaterMarkText (void)
{
    uint8_t HighWatermarkMin;

    uint8_t HighWatermarkMax;

    HighWatermarkMin = SdCardHighWatermark_Write_Min;
    HighWatermarkMax = SdCardHighWatermark_Write_Max;

    if (5 > HighWatermarkMin)   // Set lower limit
    {
        HighWatermarkMin = 10;
    }
    if (90 < HighWatermarkMax)  // Set higher limit
    {
        HighWatermarkMax = 90;
    }

    ui->HVSdCardHighWaterMark->setText (QString ("%1").
                                        sprintf
                                        ("The the unwritten area after plugin is\nbetween %d %% and %d %% of the sd card size",
                                         HighWatermarkMin, HighWatermarkMax));

    // Set valid input range
    ui->StartBlockSpin->setMaximum (HighWatermarkMax - 1);
    ui->StartBlockSpin->setMinimum (HighWatermarkMin);
    ui->StartBlockSpin->setValue (HV_Setup_st.StartBlockPercent_u8);

    ui->EndBlockSpin->setMaximum (HighWatermarkMax);
    ui->EndBlockSpin->setMinimum (HighWatermarkMin + 1);
    ui->EndBlockSpin->setValue (HV_Setup_st.EndBlockPercent_u8);
}
