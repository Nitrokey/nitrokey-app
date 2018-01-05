/*
 * Copyright (c) 2012-2018 Nitrokey UG
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

#ifndef CRYPTOSTICK_APPLET_H
#define CRYPTOSTICK_APPLET_H

#include <QApplication>
#include <QMessageBox>
#include <QtGui>
#include <QMutex>
#include <QMutexLocker>

#define CRYPTOSTICK_APP_BRAND "Nitrokey App"
QString getBrand();

class AppMessageBox {
  public:
  void messageBox(const QString msg);
  void warningBox(const QString msg);
  bool yesOrNoBox(const QString msg, bool default_val);
  bool detailedYesOrNoBox(const QString msg, const QString detailed_text, bool default_val);
  static AppMessageBox* instance(){
    //C++11 static initialization is thread safe
    //In Visual Studio supported since 2015 hence mutex
      QMutexLocker locker(&mutex);
      static AppMessageBox applet;
      return &applet;
  }
private:
    AppMessageBox() :_parent(Q_NULLPTR) {}
    static QMutex mutex;
public:
    void setParent(QWidget *parent) {
        _parent = parent;
    }

private:
    QWidget *_parent;
};

AppMessageBox *csApplet();

#endif /* CRYPTOSTICK_APPLET_H */
