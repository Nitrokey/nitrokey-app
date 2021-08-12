/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-13
 * Copyright (c) 2013-2018 Nitrokey UG
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

#ifndef STICK20RESPONSEDIALOG_H
#define STICK20RESPONSEDIALOG_H

//#include "stick20-response-task.h"
#include <QDialog>
#include <QTimer>
#include <memory>


namespace Ui {
class Stick20ResponseDialog;
}

class Stick20ResponseDialog : public QDialog {
  Q_OBJECT

public :
  explicit Stick20ResponseDialog(QWidget *parent = 0);
  virtual ~Stick20ResponseDialog();

  enum class Type{
    none, wheel, progress_bar
  };

public slots:
  void updateOperationInProgressBar(int p);
  void on_ShortOperationBegins(QString msg);
  void on_ShortOperationEnds();

private:
  Ui::Stick20ResponseDialog *ui;
  std::shared_ptr<QMovie> ProgressMovie;
  bool initialized = false;
  void init_long_operation();
  void checkStick20StatusDialog();
  void set_window_type(Type type, bool no_debug, QString text);

  void center_window();

  Type current_type = Type::none;
};

#endif // STICK20RESPONSEDIALOG_H
