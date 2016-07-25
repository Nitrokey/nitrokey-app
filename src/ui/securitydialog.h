#ifndef SECURITYDIALOG_H
#define SECURITYDIALOG_H

#include <QDialog>

namespace Ui {
class securitydialog;
}

class securitydialog : public QDialog {
  Q_OBJECT public : explicit securitydialog(QWidget *parent = 0);
  ~securitydialog();

private slots:
  void on_ST_CheckBox_toggled(bool checked);

  void on_ST_OkButton_clicked();

private:
  Ui::securitydialog *ui;
};

#endif // SECURITYDIALOG_H
