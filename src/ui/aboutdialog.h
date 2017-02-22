#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QMutex>
#include <QThread>
#include <memory>
#include "src/utils/bool_values.h"

#include "src/version.h"

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
            std::string cardSerial;
            int passwordRetryCount;
            int userPasswordRetryCount;
          struct {
            bool active;
            struct {
                bool plain;
                bool encrypted;
                bool hidden;
                bool plain_RW;
              bool encrypted_RW;
              bool hidden_RW;
            } volume_active;
            struct{
              int size_GB = 0;
              bool is_new;
              bool filled_with_random;
              uint32_t id;
            } sdcard;
            bool keys_initiated;
            uint8_t filled_with_random;
            bool firmware_locked;
            uint32_t smartcard_id;
            struct {
              bool status = false;
              int progress = 0;
            } long_operation;
          } storage;
        } devdata;
    public slots:
        void fetch_device_data();
    signals:
        void finished(bool connected);
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
  void update_device_slots(bool connected);

private:
  std::shared_ptr<Ui::AboutDialog> ui;
  AboutDialogUI::Worker worker;
  QThread worker_thread;

  void fixWindowGeometry();
};

#endif // ABOUTDIALOG_H
