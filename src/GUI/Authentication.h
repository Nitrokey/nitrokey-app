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
