#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include "device.h"
#include <QDialog>

#define GUI_VERSION "0.4.0" // 0.0 = development version,last prod
                            // version was 0.6

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog {
  Q_OBJECT public : explicit AboutDialog(Device *global_cryptostick, QWidget *parent = 0);
  ~AboutDialog();
  void showStick20Configuration(void);
  void showStick10Configuration(void);
  void showNoStickFound(void);
  void hideStick20Menu(void);
  void showStick20Menu(void);
  void hidePasswordCounters(void);
  void showPasswordCounters(void);
  void hideWarning(void);
  void showWarning(void);
  Device *cryptostick;

private slots:
  void on_ButtonOK_clicked();

  void on_ButtonStickStatus_clicked();

private:
  Ui::AboutDialog *ui;
};

#endif // ABOUTDIALOG_H
