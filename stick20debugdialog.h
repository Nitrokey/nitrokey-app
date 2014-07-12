/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-12
*
* This file is part of GPF Crypto Stick 2
*
* GPF Crypto Stick 2 is free software: you can redistribute it and/or modify
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


#ifndef DEBUGDIALOG_H
#define DEBUGDIALOG_H

#include <QDialog>
#include "device.h"

namespace Ui {
class DebugDialog;
}

class DebugDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit DebugDialog(QWidget *parent = 0);
    ~DebugDialog();

    QString DebugText;
    
    void SetNewText(QString Text);

    Device *cryptostick;

private slots:
    void UpdateDebugText ();

    void on_pushButton_clicked();

private:
    Ui::DebugDialog *ui;

    QTimer *RefreshTimer;

};

#endif // DEBUGDIALOG_H
