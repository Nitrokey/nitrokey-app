/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-12
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

#include "stick20debugdialog.h"
#include "ui_stick20debugdialog.h"

#include "mcvs-wrapper.h"

#include "src/utils/bool_values.h"

#include <QDateTime>
#include <QMenu>
#include <QTimer>
#include <QtWidgets>

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local defines

*******************************************************************************/

class OwnSleep : public QThread {
public:
  static void usleep(unsigned long usecs) { QThread::usleep(usecs); }
  static void msleep(unsigned long msecs) { QThread::msleep(msecs); }
  static void sleep(unsigned long secs) { QThread::sleep(secs); }
};

#define CS20_DEBUG_DIALOG_POLL_TIME 200     // in ms
#define CS20_DEBUG_DIALOG_POLL_TIME_FAST 10 // in ms

// #define LOCAL_DEBUG // activate for debugging

/*******************************************************************************

  DebugDialog

  Constructor DebugDialog

  Init the debug output dialog

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

DebugDialog::DebugDialog(QWidget *parent) : QDialog(parent), ui(new Ui::DebugDialog) {
  bool ret;

  static int GUIOrginalSizeY = 0;

  ui->setupUi(this);

  if (0 == GUIOrginalSizeY) {
    GUIOrginalSizeY = ui->TextGUI->size().height();
  }

  // Create timer for polling the cryptostick 2.0
  RefreshTimer = new QTimer(this);

  ret = connect(RefreshTimer, SIGNAL(timeout()), this, SLOT(UpdateDebugText()));

  RefreshTimer->start(CS20_DEBUG_DIALOG_POLL_TIME);

  ui->TextGUI->clear();
//  ui->TextGUI->appendPlainText(DebugText_GUI);

  ui->TextStick->clear();
//  ui->TextStick->appendPlainText(DebugText_Stick20);

  bool DebugingStick20PoolingActive = FALSE;
  if (FALSE == DebugingStick20PoolingActive) // When no stick debugging is
                                             // active, resize the GUI
                                             // window
  {
    ui->TextStick->hide();
    ui->label_OutputStick->hide();
    ui->TextGUI->resize(ui->TextGUI->width(), GUIOrginalSizeY + ui->TextStick->height() + 20);
  } else {
    ui->TextStick->show();
    ui->label_OutputStick->show();
    ui->TextGUI->resize(ui->TextGUI->width(), GUIOrginalSizeY);
  }

  if (ret) {
  } // Fix warnings
}

/*******************************************************************************

  ~DebugDialog

  Destructor DebugDialog

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

DebugDialog::~DebugDialog() {
  delete ui;

  RefreshTimer->stop();

  delete RefreshTimer;
}

/*******************************************************************************

  UpdateDebugText

  Get debug response from cryptostick 2.0 and update the dialog

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void DebugDialog::UpdateDebugText() {
  static int nCallCounter = 0;

//  if (TRUE == DebugingStick20PoolingActive) {
    // Send request
//    cryptostick->stick20GetDebugData();

    // Poll data from stick 20
//    Response *stick20Response = new Response();
//
//    stick20Response->DebugResponseFlag = FALSE; // Don't log response
//    stick20Response->getResponse(cryptostick);

//    if (TRUE == DebugTextHasChanged_Stick20) {
//      RefreshTimer->start(CS20_DEBUG_DIALOG_POLL_TIME_FAST);
//    } else {
//      RefreshTimer->start(CS20_DEBUG_DIALOG_POLL_TIME);
//    }
//  }

#ifdef LOCAL_DEBUG
  { // For debugging
    char text[1000];

    sprintf(text, "Calls %5d, Bytes %7d", nCallCounter, DebugTextlen_Stick20);

    ui->label->setText(text);
  }
#endif

  // Check for auto scroll activ
//  if (0 != ui->checkBox->checkState()) {
//    if (TRUE == DebugTextHasChanged_GUI) {
//      STRCAT(DebugText_GUI, sizeof(DebugText_GUI), DebugNewText_GUI);
//      ui->TextGUI->moveCursor(QTextCursor::End);
//      ui->TextGUI->insertPlainText(DebugNewText_GUI);
//      DebugNewText_GUI[0] = 0;
//      DebugNewTextLen_GUI = 0;
//      DebugTextHasChanged_GUI = FALSE;
//      ui->TextGUI->moveCursor(QTextCursor::End);
//    }

//    if (TRUE == DebugTextHasChanged_Stick20) {
//      STRCAT(DebugText_Stick20, sizeof(DebugText_Stick20), DebugNewText_Stick20);
//      ui->TextStick->moveCursor(QTextCursor::End);
//      ui->TextStick->insertPlainText(DebugNewText_Stick20);
//      DebugNewText_Stick20[0] = 0;
//      DebugNewTextLen_Stick20 = 0;
//      ui->TextStick->moveCursor(QTextCursor::End);
//      DebugTextHasChanged_Stick20 = FALSE;
//    }
//  }

//  nCallCounter++;
}

/*******************************************************************************

  updateText

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void DebugDialog::updateText(void) {
//  if (TRUE == DebugTextHasChanged_GUI) {
////    STRCAT(DebugText_GUI, sizeof(DebugText_GUI), DebugNewText_GUI);
//    ui->TextGUI->moveCursor(QTextCursor::End);
//    ui->TextGUI->insertPlainText(DebugNewText_GUI);
////    DebugNewText_GUI[0] = 0;
////    DebugNewTextLen_GUI = 0;
//
//    ui->TextGUI->moveCursor(QTextCursor::End);
//    DebugTextHasChanged_GUI = FALSE;
//  }

//  if (TRUE == DebugTextHasChanged_Stick20) {
//    STRCAT(DebugText_Stick20, sizeof(DebugText_Stick20), DebugNewText_Stick20);
//    ui->TextStick->moveCursor(QTextCursor::End);
//    ui->TextStick->insertPlainText(DebugNewText_Stick20);
//    DebugNewText_Stick20[0] = 0;
//    DebugNewTextLen_Stick20 = 0;
//
//    ui->TextStick->moveCursor(QTextCursor::End);
//    DebugTextHasChanged_Stick20 = FALSE;
//  }
}

/*******************************************************************************

  on_pushButton_clicked

  Excecute when update button was pressed

  Reviews
  Date      Reviewer        Info
  12.08.13  RB              First review

*******************************************************************************/

void DebugDialog::on_pushButton_clicked() {
//  if (TRUE == DebugTextHasChanged_GUI) {
//    STRCAT(DebugText_GUI, sizeof(DebugText_GUI), DebugNewText_GUI);
//    ui->TextGUI->moveCursor(QTextCursor::End);
//    ui->TextGUI->insertPlainText(DebugNewText_GUI);
//    DebugNewText_GUI[0] = 0;
//    DebugNewTextLen_GUI = 0;
//
//    ui->TextGUI->moveCursor(QTextCursor::End);
//    DebugTextHasChanged_GUI = FALSE;
//  }
//
//  if (TRUE == DebugTextHasChanged_Stick20) {
//    STRCAT(DebugText_Stick20, sizeof(DebugText_Stick20), DebugNewText_Stick20);
//    ui->TextStick->moveCursor(QTextCursor::End);
//    ui->TextStick->insertPlainText(DebugNewText_Stick20);
//    DebugNewText_Stick20[0] = 0;
//    DebugNewTextLen_Stick20 = 0;
//
//    ui->TextStick->moveCursor(QTextCursor::End);
//    DebugTextHasChanged_Stick20 = FALSE;
//  }
}
