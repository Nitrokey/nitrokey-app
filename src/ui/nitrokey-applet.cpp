/*
 * Copyright (c) 2014-2018 Nitrokey UG
 *
 * This file is part of Nitrokey App.
 *
 * Nitrokey App is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey App is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey App. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0
 */

#include <QApplication>
#include <QGridLayout>
#include <QMessageBox>
#include <QtGui>

#include "nitrokey-applet.h"
#include "src/GUI/ManageWindow.h"

QMutex AppMessageBox::mutex;

AppMessageBox *csApplet(){
    return AppMessageBox::instance();
}

void AppMessageBox::warningBox(const QString& msg) {
  QMessageBox::warning(_parent, getBrand(), msg, QMessageBox::Ok);
}

void AppMessageBox::messageBox(const QString& msg) {
  QMessageBox::information(_parent, getBrand(), msg, QMessageBox::Ok);
}

bool AppMessageBox::yesOrNoBox(const QString& msg, bool default_val) {
  QMessageBox::StandardButton default_btn = default_val ? QMessageBox::Yes : QMessageBox::No;

  // FIXME make it modal and always on top, or bring it up periodically

  auto buttons = QMessageBox::Yes | QMessageBox::No;
  bool b =
      QMessageBox::question(_parent, getBrand(), msg,
                            buttons, default_btn) == QMessageBox::Yes;
  return b;
}

bool AppMessageBox::detailedYesOrNoBox(const QString& msg, const QString& detailed_text, bool default_val) {
  QMessageBox *msgBox = new QMessageBox(QMessageBox::Question, getBrand(), msg, QMessageBox::Yes | QMessageBox::No,
                      _parent);

  msgBox->setDetailedText(detailed_text);
  // Turns out the layout box in the QMessageBox is a grid
  // You can force the resize using a spacer this way:
  QSpacerItem *horizontalSpacer =
      new QSpacerItem(400, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  QGridLayout *layout = (QGridLayout *)msgBox->layout();

  layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
  msgBox->setDefaultButton(default_val ? QMessageBox::Yes : QMessageBox::No);

//  msgBox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
  ManageWindow::moveToCenter(msgBox);
  ManageWindow::bringToFocus(msgBox);

  bool b = msgBox->exec() == QMessageBox::Yes;
  return b;
}

AppMessageBox *AppMessageBox::instance() {
    //C++11 static initialization is thread safe
    //In Visual Studio supported since 2015 hence mutex
    QMutexLocker locker(&mutex);
    static AppMessageBox applet;
    return &applet;
}

QString getBrand() {
  return QStringLiteral(CRYPTOSTICK_APP_BRAND);
}

