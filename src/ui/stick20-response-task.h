#ifndef STICK20RESPONSETASK_H
#define STICK20RESPONSETASK_H

#include "device.h"
#include "nitrokey-applet.h"
#include "response.h"
#include <QWidget>

class Stick20ResponseTask : public QObject {
  Q_OBJECT public : Stick20ResponseTask(QWidget *parent, Device *Cryptostick20,
                                        QSystemTrayIcon *MainWndTrayIcon);
  ~Stick20ResponseTask();

  void done(int Status);
  void GetResponse(void);
  void NoStopWhenStatusOK(void);
  void checkStick20Status(void);

  QWidget *Stick20ResponseTaskParent;

  Device *cryptostick;
  QSystemTrayIcon *trayIcon;
  Response *stick20Response;

  int ActiveCommand;
  bool FlagNoStopWhenStatusOK;
  int ResultValue;
  int EndFlag;
  int Counter_u32;
  int retStick20Respone;

private:
  void ShowIconMessage(QString IconText);
  void setDescription(void);
};

#endif // STICK20RESPONSETASK_H
