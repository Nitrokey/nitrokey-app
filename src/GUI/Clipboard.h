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

#ifndef NITROKEYAPP_CLIPBOARD_H
#define NITROKEYAPP_CLIPBOARD_H

#include <QObject>
#include <QClipboard>
#include <QString>

class Clipboard : public QObject
{
Q_OBJECT
public:
    Clipboard(QObject *parent);
    void copyToClipboard(QString text, int time=60);

    virtual ~Clipboard();

private:
    QClipboard *clipboard;
    uint lastClipboardTime;
    QString secretInClipboard;
private slots:
    void checkClipboard_Valid(bool force_clear=false);

};


#endif //NITROKEYAPP_CLIPBOARD_H
