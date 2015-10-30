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

#include "stick20updatedialog.h"
#include "ui_stick20updatedialog.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/


/*******************************************************************************

  UpdateDialog

  Constructor UpdateDialog

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

UpdateDialog::UpdateDialog (QWidget * parent):
QDialog (parent), ui (new Ui::UpdateDialog)
{
    ui->setupUi (this);
}

/*******************************************************************************

  UpdateDialog

  Destructor UpdateDialog

  Reviews
  Date      Reviewer        Info
  13.08.13  RB              First review

*******************************************************************************/

UpdateDialog::~UpdateDialog ()
{
delete ui;
}
