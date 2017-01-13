#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QMutex>
#include <QThread>
#include <memory>
#include "src/utils/bool_values.h"

#define GUI_VERSION "0.6.2"


namespace Ui {
class AboutDialog;
}

namespace AboutDialogUI{
    class Worker : public QObject
    {
    Q_OBJECT
    public:
        QMutex mutex;
        struct {
            int majorFirmwareVersion;
            int minorFirmwareVersion;
            int sd_size_GB = 0;
            std::string cardSerial;
            int passwordRetryCount;
            int userPasswordRetryCount;
        } devdata;
    public slots:
        void fetch_device_data();
    signals:
        void finished();
    };
}

class AboutDialog : public QDialog {
  Q_OBJECT
  public :
  explicit AboutDialog(QWidget *parent = 0);
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

private slots:
  void on_ButtonOK_clicked();
  void on_ButtonStickStatus_clicked();
  void update_device_slots();

private:
  std::shared_ptr<Ui::AboutDialog> ui;
  AboutDialogUI::Worker worker;
  QThread worker_thread;
};

#endif // ABOUTDIALOG_H
