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

#ifndef MatrixPasswordDialog_H
#define MatrixPasswordDialog_H

#include <QFrame>
#include <QDialog>
#include "device.h"

namespace Ui {
class MatrixPasswordDialog;
}

class MatrixPasswordDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit MatrixPasswordDialog(QWidget *parent = 0);
    ~MatrixPasswordDialog();

    void InitSecurePasswordDialog (void);
    bool CopyMatrixPassword(char *Password,int len);

    Device *cryptostick;
    int     PasswordLen;
    bool    SetupInterfaceFlag;
//    int     PasswordKind;
    
private slots:
    void on_pushButton_0_clicked();
    void on_pushButton_1_clicked();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();
    void on_pushButton_6_clicked();
    void on_pushButton_7_clicked();
    void on_pushButton_8_clicked();
    void on_pushButton_9_clicked();
    void CheckGetPasswordMatrix(void);


private:
    void addChar (uint8_t *Text, uint8_t AddChar);
    void sendPasswordKey (char *Password);
    void SendMatrixRowDataToStick20();

    bool eventButton(QEvent *event,int Number);

    Ui::MatrixPasswordDialog *ui;

    void PaintMatrix(QFrame *frame);

    bool eventFilter(QObject *target, QEvent *event);
    void mousePressEvent ( QMouseEvent * e );
    void mouseMoveEvent ( QMouseEvent * e );

    void RowSelected(int SelectedRow);

    int  SetupPasswordLen;
    int  HideMatrix;

    // Vars for entering password
    int  InputPasswordLength;
    int  InputPasswordLengthPointer;
    unsigned char SelectedColumns[OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS+1]; // + 1 for 0 at end of string


    // Vars for setup
    bool SetupMode;
    int  HighLightVerticalBar;
    int  SetupModeActiveRow;
    int  SelectedRowCounter;
    int  Stick20MatrixDataReceived;
    unsigned char SelectedRows[OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS+1];
    QTimer *WaitForMatrixTimer;


};

#endif // MatrixPasswordDialog_H
