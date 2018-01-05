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

#ifndef NITROKEYAPP_AUTHENTICATION_H
#define NITROKEYAPP_AUTHENTICATION_H


#include <QObject>
#include <QString>

class Authentication : public QObject
{
Q_OBJECT
public:
    enum Type{ADMIN, USER, FIRMWARE, HIDDEN_VOLUME};
    Authentication(QObject *parent, Type _type);
    ~Authentication();
    bool authenticate();
    const QByteArray getTempPassword();
    std::string getPassword();
    bool isAuthenticated() const;
private:
    Q_DISABLE_COPY(Authentication);
    quint64 authenticationValidUntilTime;
    QByteArray tempPassword;
    QByteArray generateTemporaryPassword() const;
    const Type type;
    quint64 getCurrentTime() const;
    void markAuthenticated();
private slots:
    void clearTemporaryPassword(bool force=false);
    void clearTemporaryPasswordForced();
};


#endif //NITROKEYAPP_AUTHENTICATION_H
