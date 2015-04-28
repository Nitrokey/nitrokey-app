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

#ifndef STICK20RESPONSEDIALOG_H
#define STICK20RESPONSEDIALOG_H

#include <QDialog>
#include <QTimer>
#include "device.h"
#include "response.h"
#include "stick20-response-task.h"


#define STICK20_DEBUG_TEXT_LEN          600000

#ifdef __cplusplus
extern "C" {
#endif
    extern char DebugText_GUI[STICK20_DEBUG_TEXT_LEN];
    extern int  DebugTextlen_GUI;
    extern int  DebugingActive;
    extern int  DebugingStick20PoolingActive;

    void DebugAppendTextGui (const char *Text);
#ifdef __cplusplus
} // extern "C"
#endif


namespace Ui {
    class Stick20ResponseDialog;
}

class Stick20ResponseDialog : public QDialog
{
    Q_OBJECT

    
public:
    Stick20ResponseTask *Stick20Task;

    explicit Stick20ResponseDialog(QWidget *parent = 0,Stick20ResponseTask *Stick20TaskPointer = 0);
    ~Stick20ResponseDialog();

    void checkStick20StatusDebug(Response *stick20Response,int Status);
    void showStick20Configuration (int Status);

    QTimer *pollStick20Timer;

private:
    Ui::Stick20ResponseDialog *ui;

private slots:
    void checkStick20StatusDialog();
};

#endif // STICK20RESPONSEDIALOG_H
