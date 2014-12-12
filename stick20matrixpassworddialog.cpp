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


#include "stick20matrixpassworddialog.h"
#include "ui_stick20matrixpassworddialog.h"
#include "response.h"
#include "stick20responsedialog.h"
#include "stick20hid.h"
#include "cryptostick-applet.h"

#include <QTimer>
#include <QPainter>
#include <QMouseEvent>

/*******************************************************************************

 Local defines

*******************************************************************************/

#define STICK20_PASSWORD_MATRIX_DATA_LEN                  100

#define STICK20_PASSWORD_MATRIX_STATUS_IDLE                 0
#define STICK20_PASSWORD_MATRIX_STATUS_GET_NEW_BLOCK        1
#define STICK20_PASSWORD_MATRIX_STATUS_NEW_BLOCK_RECEIVED   2

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

  MatrixPasswordDialog

  Constructor MatrixPasswordDialog

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

MatrixPasswordDialog::MatrixPasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MatrixPasswordDialog)
{
    ui->setupUi(this);
    QPainter painter(ui->frame);
    QRect Rect;
    bool ret;
    int i;
    int height,width;


    ui->frame->installEventFilter(this);
    ui->frame->setMouseTracking(true);

    ui->pushButton_0->installEventFilter(this);
    ui->pushButton_1->installEventFilter(this);
    ui->pushButton_2->installEventFilter(this);
    ui->pushButton_3->installEventFilter(this);
    ui->pushButton_4->installEventFilter(this);
    ui->pushButton_5->installEventFilter(this);
    ui->pushButton_6->installEventFilter(this);
    ui->pushButton_7->installEventFilter(this);
    ui->pushButton_8->installEventFilter(this);
    ui->pushButton_9->installEventFilter(this);

    ui->pushButton_Send->installEventFilter(this);
    ui->pushButton_Exit->installEventFilter(this);


    height = ui->frame->size().height();
    width  = ui->frame->size().width();

   //    ui->pushButton_0->set

    Rect = ui->frame->geometry();


    i = 0;
    ui->pushButton_0->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_1->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_2->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_3->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_4->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_5->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_6->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_7->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_8->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;
    ui->pushButton_9->setGeometry(QRect(Rect.x()+width * i / 10 + width / 20 - 10, 355, 20, 20));  i++;

// Hardware access var isn't filled
    cryptostick = NULL;

// Init Timer
    WaitForMatrixTimer = new QTimer(this);

    ret = connect(WaitForMatrixTimer, SIGNAL(timeout()), this, SLOT(CheckGetPasswordMatrix()));

    WaitForMatrixTimer->start(100);

    Stick20MatrixDataReceived = 0;
    HID_Stick20MatrixPasswordData_st.StatusFlag_u8 = STICK20_PASSWORD_MATRIX_STATUS_IDLE;
    if(ret){}//Fix warnings
    if(height){}//Fix warnings
}

/*******************************************************************************

  MatrixPasswordDialog

  Destructor MatrixPasswordDialog

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

MatrixPasswordDialog::~MatrixPasswordDialog()
{
    delete ui;

    WaitForMatrixTimer->stop();

    delete WaitForMatrixTimer;
}

/*******************************************************************************

  InitSecurePasswordDialog

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::InitSecurePasswordDialog()
{
    bool        ret;
    bool        waitForAnswerFromStick20;
    int         i;
    int         i1;
    QByteArray  passwordString;
    QString     OutputText;

    waitForAnswerFromStick20 = FALSE;

    HighLightVerticalBar     = -1;
    HideMatrix               = false;

    SetupMode                = SetupInterfaceFlag; //false;
    SetupPasswordLen         = PasswordLen;

    ui->frame->setMouseTracking(true);

    SetupModeActiveRow        = -1;
    SelectedRowCounter        = 0;
    InputPasswordLength       = PasswordLen;
    Stick20MatrixDataReceived = 0;

    InputPasswordLengthPointer = 0;

    // No Stick - no work ?
    if (false == cryptostick->isConnected){
        csApplet->warningBox("MatrixPasswordDialog: No cryptostick 2.0 connected!");
        return;
    }

    if (TRUE == SetupMode)
    {
        OutputText = "Select row " + QString("%1").arg(1);
        ui->label->setText(OutputText);

        // Set setup matrix data
        for (i=0;i<10;i++)
        {
            for (i1=0;i1<10;i1++)
            {
                HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[i1*10+i] = i;
            }
        }
    }
    else
    {
        // Send get password matrix request
        ret = cryptostick->stick20GetPasswordMatrix ();

        ui->label->setText("Waiting for matrix data generated by cryptostick 2.0");
    }
    if(ret){}//Fix warnings
    if(waitForAnswerFromStick20){}//Fix warnings
}

/*******************************************************************************

  CheckGetPasswordMatrix

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::CheckGetPasswordMatrix(void)
{
    if (NULL == cryptostick)
    {
        return;
    }

    Response *stick20Response = new Response();


// Get HID data from stick 2.0
    stick20Response->getResponse(cryptostick);

// Get all data ?
    if (STICK20_PASSWORD_MATRIX_STATUS_NEW_BLOCK_RECEIVED == HID_Stick20MatrixPasswordData_st.StatusFlag_u8)
    {
        Stick20MatrixDataReceived = 1;
        WaitForMatrixTimer->stop();
        ui->label->setText("Select column 1");
        ui->frame->repaint();
    }

    // Todo check for timeout
}

/*******************************************************************************

  addChar

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::addChar (uint8_t *Text, uint8_t AddChar)
{
    int n;

    n = strlen ((char *)Text);

    Text[n]   = AddChar;
    Text[n+1] = 0;
}

/*******************************************************************************

  sendPasswordKey

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::sendPasswordKey (char *Password)
{
    QString  OutputText;
    bool     ret;

// Store the entry
    SelectedColumns[InputPasswordLengthPointer] = Password[0];
    InputPasswordLengthPointer++;

// Enter the next char
    if (InputPasswordLengthPointer < OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS)
    {
        OutputText = "Select column " + QString("%1").arg(InputPasswordLengthPointer+1);
        ui->label->setText(OutputText);
    }
    else
    {
        // No, send the data to stick 2.0

        SelectedColumns[InputPasswordLengthPointer] = 0;
        ret = cryptostick->stick20SendPasswordMatrixPinData (SelectedColumns);
        if (TRUE == ret)
        {
        }

        // Exit Dialog
        done(true);
    }
}

/*******************************************************************************

  on_pushButton_0_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_0_clicked()
{
    sendPasswordKey ((char *)"0");
}

/*******************************************************************************

  on_pushButton_1_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_1_clicked()
{
    sendPasswordKey ((char *)"1");
}
/*******************************************************************************

  on_pushButton_2_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_2_clicked()
{
    sendPasswordKey ((char *)"2");
}

/*******************************************************************************

  on_pushButton_3_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_3_clicked()
{
    sendPasswordKey ((char *)"3");
}

/*******************************************************************************

  on_pushButton_4_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_4_clicked()
{
    sendPasswordKey ((char *)"4");
}

/*******************************************************************************

  on_pushButton_5_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_5_clicked()
{
    sendPasswordKey ((char *)"5");
}

/*******************************************************************************

  on_pushButton_6_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_6_clicked()
{
    sendPasswordKey ((char *)"6");
}

/*******************************************************************************

  on_pushButton_7_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_7_clicked()
{
    sendPasswordKey ((char *)"7");
}

/*******************************************************************************

  on_pushButton_8_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_8_clicked()
{
    sendPasswordKey ((char *)"8");
}

/*******************************************************************************

  on_pushButton_9_clicked

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::on_pushButton_9_clicked()
{
    sendPasswordKey ((char *)"9");
}

/*******************************************************************************

  PaintMatrix

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::PaintMatrix(QFrame *frame)
{
    int i,i1;
    QPainter painter(frame);
    QColor color;
    QFont font;
    int height,width;
    int text_height;

// Get size and text high
    height = frame->size().height();
    width  = frame->size().width();
    text_height = painter.fontMetrics().height();

// Is matrix data ready ?
    if (FALSE == SetupMode)
    {
        if (0 == Stick20MatrixDataReceived)
        {
            color.setRgb(200,200,200);
            painter.fillRect(0,0,width,height,color);

            color.setRgb(255,255,255);
            painter.setPen(color);
            font = painter.font();
            font.bold();
            font.setPixelSize(22);
            painter.setFont (font);
            painter.drawText(20,height/2,"Waiting for matrix data generated by cryptostick 2.0");
            return;
        }
    }


// Hide the matrix, not in setup mode
    if (((int)true == HideMatrix) && (false == SetupMode))
    {
        color.setRgb(200,200,200);
        painter.fillRect(0,0,width,height,color);

        color.setRgb(255,255,255);
        painter.setPen(color);
        font = painter.font();
        font.bold();
        font.setPixelSize(22);
        painter.setFont (font);
        painter.drawText(150,height/2,"Move cursor out of frame");
        return;
    }

// Paint horizontal bar
    for (i=0;i<10;i++)
    {
        color.setRgb(100+50*(i%2),100+50*(i%2),100+50*(i%2));
        painter.fillRect(0,height*i/10,width,height/10,color);
    }

    if ((false == HideMatrix) && (false == SetupMode))
    {
    // Paint vertical bar
        if (-1 != HighLightVerticalBar)
        {
            for (i=0;i<10;i++)
            {
                color.setRgb(50+50*(i%2),50+50*(i%2),50+50*(i%2));
                painter.fillRect(width*HighLightVerticalBar/10,height*i/10,width/10,height/10,color);
            }
        }
    }

    if (true == SetupMode)
    {
    // Mark a row in the setup mode
        if (-1 != SetupModeActiveRow)
        {
            color.setRgb(200,200,200);
            painter.fillRect(0,height/10*SetupModeActiveRow,width,height/10,color);
        }
    }

// Print text
    for (i=0;i<10;i++)
    {
        for (i1=0;i1<10;i1++)
        {  
            QString text;
            color.setRgb(255,255,255);
            painter.setPen(color);

            font = painter.font();
            font.bold();
            font.setPixelSize(20);
            painter.setFont (font);

//            painter.drawText(width*i/10+(width/20),(height*(i1+1)/10)-text_height,"1");
            text = '0' + HID_Stick20MatrixPasswordData_st.PasswordMatrix_u8[i*10+i1];
            painter.drawText(width*i/10+(width/20)-6,(height*(i1+1)/10)-text_height+2,text);
        }
    }
}

/*******************************************************************************

  eventButton

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool MatrixPasswordDialog::eventButton(QEvent *event,int Number)
{
    if (event->type() == QEvent::Enter)
    {
        HighLightVerticalBar = Number;
        ui->frame->repaint();
        return true;
    }
    if (event->type() == QEvent::Leave)
    {
        HighLightVerticalBar = -1;
        ui->frame->repaint();
        return true;
    }
    return false;
}

/*******************************************************************************

  RowSelected

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::RowSelected(int SelectedRow)
{
    QString OutputText;

    SelectedRows[SelectedRowCounter] = '0' + SelectedRow; // Store as Ascii
    SelectedRowCounter++;

    if (SelectedRowCounter < OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS)
    {
        OutputText = "Select row " + QString("%1").arg(SelectedRowCounter+1);
        ui->label->setText(OutputText);
    }
    else
    {
        csApplet->warningBox("Maximum length reached");

        SendMatrixRowDataToStick20 ();
        done (true);
    }
}

/*******************************************************************************

  eventFilter

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool MatrixPasswordDialog::eventFilter(QObject *target, QEvent *event)
{
    bool ret;

    if (false == SetupMode)
    {
        if (target == ui->pushButton_0)
        {
            return (eventButton(event,0));
        }
        if (target == ui->pushButton_1)
        {
            return (eventButton(event,1));
        }
        if (target == ui->pushButton_2)
        {
            return (eventButton(event,2));
        }
        if (target == ui->pushButton_3)
        {
            return (eventButton(event,3));
        }
        if (target == ui->pushButton_4)
        {
            return (eventButton(event,4));
        }
        if (target == ui->pushButton_5)
        {
            return (eventButton(event,5));
        }
        if (target == ui->pushButton_6)
        {
            return (eventButton(event,6));
        }
        if (target == ui->pushButton_7)
        {
            return (eventButton(event,7));
        }
        if (target == ui->pushButton_8)
        {
            return (eventButton(event,8));
        }
        if (target == ui->pushButton_9)
        {
            return (eventButton(event,9));
        }
    }
/*
    if (target == ui->plainTextEdit)
    {

        if (event->type() == QEvent::MouseMove)
        {
            QPoint Pos;
            QMouseEvent *e;

            e = (QMouseEvent *)event;

        }
        if (event->type() == QEvent::MouseButtonRelease)
        {

        }

    }
*/
    if (target == ui->frame)
    {
        if (event->type() == QEvent::Paint)
        {
            PaintMatrix ((QFrame*)target);
        }
        // In the normal mode hide the data when the cursor get on the frame
        if (false == SetupMode)
        {
            SetupModeActiveRow = -1;
            if (event->type() == QEvent::Enter)
            {
                HideMatrix = true;
                ui->frame->repaint();
                return true;
            }
            if (event->type() == QEvent::Leave)
            {
                HideMatrix = false;
                ui->frame->repaint();
                return true;
            }
        }
        else
        {
            // Setup matrix mode
//            test =   ui->frame->hasMouseTracking();
            // Get the row the mouse pointer
            if (event->type() == QEvent::MouseMove)
            {
                QMouseEvent *e;
                e = (QMouseEvent *)event;
                SetupModeActiveRow = e->pos().y() * 10 / ui->frame->size().height();
                ui->frame->repaint();
                return true;
            }
            // Store the selected row
            if (event->type() == QEvent::MouseButtonRelease)
            {
                if (OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS > SelectedRowCounter)
                {
                  RowSelected(SetupModeActiveRow);
                }
                return true;
            }
            if (event->type() == QEvent::Enter)
            {
                QMouseEvent *e;
                e = (QMouseEvent *)event;
                SetupModeActiveRow = e->pos().y() * 10 / ui->frame->size().height();
                ui->frame->repaint();
                return true;
            }
            if (event->type() == QEvent::Leave)
            {
                SetupModeActiveRow = -1;
                ui->frame->repaint();
                return true;
            }
        }
    }

// Send button
    if (target == ui->pushButton_Send)
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            if (false == SetupMode)
            {
                // Yes, send the pin data to stick 2.0
                SelectedColumns[InputPasswordLengthPointer] = 0;
                ret = cryptostick->stick20SendPasswordMatrixPinData (SelectedColumns);
                if (TRUE == ret)
                {
                }
            }
            else
            {
                // Send setupdata
                SendMatrixRowDataToStick20();
            }

            // Exit Dialog
            done(true);

            return true;
        }
    }

// Exit button
    if (target == ui->pushButton_Exit)
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            done (FALSE);
            return true;
        }
    }
    return QDialog::eventFilter(target, event);
}

/*******************************************************************************

  mousePressEvent

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::mousePressEvent ( QMouseEvent * e )
{
    return QDialog::mousePressEvent(e);
}
/*******************************************************************************

  mouseMoveEvent

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::mouseMoveEvent  ( QMouseEvent * e )
{
    return QDialog::mouseMoveEvent (e);
}
/*******************************************************************************

  SendMatrixRowDataToStick20

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

void MatrixPasswordDialog::SendMatrixRowDataToStick20()
{
    bool ret;
    unsigned char Data[OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS+2];

    Data[0] = 'P';

    memcpy (&Data[1],SelectedRows,SelectedRowCounter);
    Data[1+SelectedRowCounter] = 0;

    ret = cryptostick->stick20SendPasswordMatrixSetup (Data);
    if (TRUE == ret)
    {
    }

// Wait vor response
    Stick20ResponseTask ResponseTask(this,cryptostick,NULL);
    ResponseTask.GetResponse ();

/*
    Stick20ResponseDialog ResponseDialog(this);

    ResponseDialog.setModal (TRUE);

    ResponseDialog.cryptostick=cryptostick;

    ResponseDialog.exec();
*/
}

/*******************************************************************************

  CopyMatrixPassword

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

bool MatrixPasswordDialog::CopyMatrixPassword(char *Password,int len)
{
    if (len-1 <= InputPasswordLengthPointer)
    {
        return (FALSE);
    }

    memcpy (Password,SelectedColumns,InputPasswordLengthPointer);
    Password[InputPasswordLengthPointer] = 0;

    return (TRUE);
}
