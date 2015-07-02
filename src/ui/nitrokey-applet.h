#ifndef CRYPTOSTICK_APPLET_H
#define CRYPTOSTICK_APPLET_H

#include <QtGui>
#include <QApplication>
#include <QMessageBox>

#define CRYPTOSTICK_APP_BRAND "Nitrokey App"
QString getBrand ();

class CryptostickApplet:QObject
{
  Q_OBJECT public:
    CryptostickApplet ();
    void messageBox (const QString & msg, QWidget * parent = 0);
    void warningBox (const QString & msg, QWidget * parent = 0);
    bool yesOrNoBox (const QString & msg, QWidget * parent =
                     0, bool default_val = true);
    bool detailedYesOrNoBox (const QString & msg,
                             const QString & detailed_text, QWidget * parent =
                             0, bool default_val = true);

};

extern CryptostickApplet* csApplet;

#endif /* CRYPTOSTICK_APPLET_H */
