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

Authentication::Authentication(QObject *parent) : QObject(parent) {

}

bool Authentication::authenticate(){
  bool authenticationSuccess = false;

  const auto validation_period = 10 * 60 * 1000; //TODO move to field, add to ctr as param
  if (!tempPassword.isEmpty() && authenticationValidUntilTime >= getCurrentTime()){
    authenticationValidUntilTime = getCurrentTime() + validation_period;
    authenticationSuccess = true;
    QTimer::singleShot(validation_period, this, SLOT(clearTemporaryPassword()));
    return authenticationSuccess;
  }

  QString password;
  do {
    PinDialog dialog(tr("Enter admin PIN"), tr("Admin PIN:"), PinDialog::PLAIN,
                     PinDialog::ADMIN_PIN); //TODO move user/admin to field, add to ctr as param
    int ok = dialog.exec();
    if (ok != QDialog::Accepted) {
      return authenticationSuccess;
    }

    //emit signal with password to authenticate (by pointer)
    auto password = dialog.getPassword();
    tempPassword = generateTemporaryPassword();
    //emit end

    //slot receiving signal
    try{
      nm::instance()->first_authenticate(password.c_str(), tempPassword.toLatin1().constData());
      //FIXME securedelete password
      authenticationValidUntilTime = getCurrentTime() + validation_period;
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

uint Authentication::getCurrentTime() const { return QDateTime::currentDateTime().toTime_t(); }

void Authentication::clearTemporaryPassword(bool force){
  if (force || authenticationValidUntilTime < getCurrentTime()) {
    tempPassword.clear(); //FIXME securely delete
  }
}


#include <QByteArray>
QString Authentication::generateTemporaryPassword() const {
  QByteArray tmp_p(25,0); //TODO remove magic number - 25 - maxlen of temp pass
  for (int i = 0; i < tmp_p.size(); i++)
    tmp_p[i] = qrand() & 0xFF;
  return tmp_p;
}

#include "core/ScopedGuard.h"
const QString Authentication::getTempPassword() {
    QString local_tempPassword = tempPassword;
    bool is_07nkpro_device = true;
  if (is_07nkpro_device){
    clearTemporaryPassword(true);
  }
  return local_tempPassword;
}