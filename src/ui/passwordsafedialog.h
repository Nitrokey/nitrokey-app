/*
 * Author: Copyright (C)  Rudolf Boeddeker  Date: 2014-08-04
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

#ifndef PASSWORDSAFEDIALOG_H
#define PASSWORDSAFEDIALOG_H

#include <QDialog>
#include "device.h"

namespace Ui
{
    class PasswordSafeDialog;
}

class PasswordSafeDialog:public QDialog
{
  Q_OBJECT public:
      explicit PasswordSafeDialog (int Slot, QWidget * parent = 0);
     ~PasswordSafeDialog ();

    Device* cryptostick;

    int delaySendTextInMs;
    int UsedSlot;

    private slots:void on_ButtonSendpassword_clicked ();

    void on_ButtonSendPW_LN_clicked ();

    void on_ButtonSendLoginname_clicked ();

    void on_ButtonOk_clicked ();

    void on_spinBoxDelay_valueChanged ();

    void on_radioCutUPaste_clicked ();

    void on_radioKeyboard_clicked ();

  private:
      Ui::PasswordSafeDialog * ui;

    QClipboard* clipboard;
};

#endif // PASSWORDSAFEDIALOG_H
