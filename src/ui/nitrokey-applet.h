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
  public:
  void messageBox(const QString msg);
  void warningBox(const QString msg);
  bool yesOrNoBox(const QString msg, bool default_val);
  bool detailedYesOrNoBox(const QString msg, const QString detailed_text, bool default_val);
    static CryptostickApplet* instance(){
        QMutexLocker locker(&mutex);
        static CryptostickApplet applet;
        return &applet;
    }
private:
    CryptostickApplet() :_parent(Q_NULLPTR) {}
    static QMutex mutex;
public:
    void setParent(QWidget *parent) {
        CryptostickApplet::_parent = parent;
    }

private:
    QWidget *_parent;
};

CryptostickApplet *csApplet();

#endif /* CRYPTOSTICK_APPLET_H */
