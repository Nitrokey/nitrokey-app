#ifndef STICK20LOCKFIRMWAREDIALOG_H
#define STICK20LOCKFIRMWAREDIALOG_H

#include <QDialog>

namespace Ui {
class stick20LockFirmwareDialog;
}

class stick20LockFirmwareDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit stick20LockFirmwareDialog(QWidget *parent = 0);
    ~stick20LockFirmwareDialog();
    
private:
    Ui::stick20LockFirmwareDialog *ui;
};

#endif // STICK20LOCKFIRMWAREDIALOG_H
