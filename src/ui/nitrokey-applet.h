#ifndef CRYPTOSTICK_APPLET_H
#define CRYPTOSTICK_APPLET_H

#include <QApplication>
#include <QMessageBox>
#include <QtGui>
#include <QMutex>
#include <QMutexLocker>

#define CRYPTOSTICK_APP_BRAND "Nitrokey App"
QString getBrand();

class AppMessageBox {
  public:
  void messageBox(const QString msg);
  void warningBox(const QString msg);
  bool yesOrNoBox(const QString msg, bool default_val);
  bool detailedYesOrNoBox(const QString msg, const QString detailed_text, bool default_val);
  static AppMessageBox* instance(){
    //C++11 static initialization is thread safe
    //In Visual Studio supported since 2015 hence mutex
      QMutexLocker locker(&mutex);
      static AppMessageBox applet;
      return &applet;
  }
private:
    AppMessageBox() :_parent(Q_NULLPTR) {}
    static QMutex mutex;
public:
    void setParent(QWidget *parent) {
        _parent = parent;
    }

private:
    QWidget *_parent;
};

AppMessageBox *csApplet();

#endif /* CRYPTOSTICK_APPLET_H */
