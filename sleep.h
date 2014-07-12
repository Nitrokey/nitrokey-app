/*
* Author: Copyright (C) Andrzej Surowiec 2012
*
*
* This file is part of GPF Crypto Stick.
*
* GPF Crypto Stick is free software: you can redistribute it and/or modify
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

#ifndef SLEEP_H
#define SLEEP_H

#include <qthread.h>

class Sleep: public QThread
{
public:
static void sleep(unsigned long secs) {
QThread::sleep(secs);
}
static void msleep(unsigned long msecs) {
QThread::msleep(msecs);
}
static void usleep(unsigned long usecs) {
QThread::usleep(usecs);
}
};

#endif // SLEEP_H
