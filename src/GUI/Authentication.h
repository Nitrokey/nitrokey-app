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
    enum Type{ADMIN, USER, FIRMWARE, HIDDEN_VOLUME};
    Authentication(QObject *parent, Type _type);
    bool authenticate();
    const QString getTempPassword();
    std::string getPassword();
private:
    Q_DISABLE_COPY(Authentication);
    quint64 authenticationValidUntilTime;
    QString tempPassword;
    QString generateTemporaryPassword() const;
    const Type type;
    uint getCurrentTime() const;
private slots:
    void clearTemporaryPassword(bool force=false);
};


#endif //NITROKEYAPP_AUTHENTICATION_H
