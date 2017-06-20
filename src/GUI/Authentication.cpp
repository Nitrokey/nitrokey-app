#include "Authentication.h"

#include <QDateTime>
#include "pindialog.h"
//#include "SecureString.h"
#include "libada.h"
#include "libnitrokey/include/NitrokeyManager.h"
using nm = nitrokey::NitrokeyManager;
#include <string>
#include "nitrokey-applet.h"
#include <QTimer>

Authentication::Authentication(QObject *parent, Type _type) : QObject(parent), type(_type) {
    tempPassword.clear();
}

std::string Authentication::getPassword() {
  const auto pinType = type==ADMIN? PinDialog::ADMIN_PIN : PinDialog::USER_PIN;
  PinDialog dialog(pinType);

  int ok = dialog.exec();
  if (ok != QDialog::Accepted) {
    return "";
  }

  return dialog.getPassword();
}

bool Authentication::authenticate(){
//  qDebug() << tempPassword.toLatin1().toHex();
  bool authenticationSuccess = false;

  const auto validation_period = 10 * 60 * 1000; //TODO move to field, add to ctr as param
  if (!tempPassword.isEmpty() && authenticationValidUntilTime >= getCurrentTime()){
    authenticationSuccess = true;
    return authenticationSuccess;
  }

  QString password;
  do {
      const auto pinType = type==ADMIN? PinDialog::ADMIN_PIN : PinDialog::USER_PIN;
      PinDialog dialog(pinType);

    int ok = dialog.exec();
    if (ok != QDialog::Accepted) {
      return authenticationSuccess;
    }

    //emit signal with password to authenticate (by pointer)
    auto password = dialog.getPassword();
//    auto tempPasswordLocal = generateTemporaryPassword();
    tempPassword = generateTemporaryPassword();
    //emit end

    //slot receiving signal
    try{
      switch (pinType){
        case PinDialog::USER_PIN:
          nm::instance()->user_authenticate(password.c_str(), tempPassword.constData());
          break;
        case PinDialog::ADMIN_PIN:
          nm::instance()->first_authenticate(password.c_str(), tempPassword.constData());
          break;
        default:
          break;
      }

      //FIXME securedelete password
      authenticationValidUntilTime = getCurrentTime() + validation_period;
      // arm the clearing of the temp password 10 ms after the deadline to
      // avoid the race condition with comparing current time and expiry time
      QTimer::singleShot(validation_period + 10, this, SLOT(clearTemporaryPassword()));
      authenticationSuccess = true;
      return authenticationSuccess;
    }
    catch (CommandFailedException &e){
      if (!e.reason_wrong_password())
        throw;
      csApplet()->warningBox(tr("Wrong PIN. Please try again."));
    }
    //emit success(true/false)
    //slot end

  } while (true);
  return authenticationSuccess;
}

quint64 Authentication::getCurrentTime() const { return QDateTime::currentMSecsSinceEpoch(); }

void Authentication::clearTemporaryPassword(bool force){
  if (force || authenticationValidUntilTime < getCurrentTime()) {
    tempPassword.clear(); //FIXME securely delete
  }
}


#include <random>
QByteArray Authentication::generateTemporaryPassword() const {
  static std::random_device rd;
  static std::mt19937 mt(rd());
  static std::uniform_int_distribution<unsigned char> dist(0, 0xFF);

  auto temporary_password_length = 25;
  QByteArray tmp_p(temporary_password_length, 0);
  auto size = tmp_p.size();
  for (int i = 0; i < size; i++){
    tmp_p[i] = dist(mt);
  }
  return tmp_p;
}

#include "core/ScopedGuard.h"

const QByteArray Authentication::getTempPassword() {
//    authenticate(); //TODO check?
    const QByteArray local_tempPassword = tempPassword;
    bool is_07nkpro_device = libada::i()->is_nkpro_07_rtm1();
    if (is_07nkpro_device) {
        clearTemporaryPassword(true);
    }
    return local_tempPassword;
}
