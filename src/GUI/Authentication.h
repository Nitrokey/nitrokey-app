//
// Created by sz on 16.01.17.
//

#ifndef NITROKEYAPP_AUTHENTICATION_H
#define NITROKEYAPP_AUTHENTICATION_H


#include <QObject>
#include <QString>

class Authentication : public QObject
{
Q_OBJECT
public:
    Authentication(QObject *parent);
    bool authenticate();

    const QString &getTempPassword() const;

private:
    quint64 authenticationValidUntilTime;
    QString tempPassword;
    QString generateTemporaryPassword() const;
private slots:
    void clearTemporaryPassword();

    uint getCurrentTime() const;
};


#endif //NITROKEYAPP_AUTHENTICATION_H
