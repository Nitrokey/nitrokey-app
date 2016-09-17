#ifndef CRYPTOSTICK_APPLET_H
#define CRYPTOSTICK_APPLET_H

#include <QApplication>
#include <QMessageBox>
#include <QtGui>
#include <QMutex>
#include <QMutexLocker>

#define CRYPTOSTICK_APP_BRAND "Nitrokey App"
QString getBrand();

class CryptostickApplet {
  public :
    void messageBox(const QString msg, QWidget *parent = 0);
  void warningBox(const QString msg, QWidget *parent = 0);
  bool yesOrNoBox(const QString msg, QWidget *parent = 0, bool default_val = true);
  bool detailedYesOrNoBox(const QString msg, const QString detailed_text, QWidget *parent = 0,
                          bool default_val = true);
    static CryptostickApplet* instance(){
        QMutexLocker locker(&mutex);
        static CryptostickApplet applet;
        return &applet;
    }
private:
    CryptostickApplet() {}
    static QMutex mutex;
};

CryptostickApplet *csApplet();

#endif /* CRYPTOSTICK_APPLET_H */
