#include "securitydialog.h"
#include "ui_securitydialog.h"

securitydialog::securitydialog (QWidget * parent):
QDialog (parent), ui (new Ui::securitydialog)
{
    ui->setupUi (this);
    ui->ST_OkButton->setDisabled (true);
}

securitydialog::~securitydialog ()
{
delete ui;
}

void securitydialog::on_ST_CheckBox_toggled (bool checked)
{
    if (true == checked)
    {
        ui->ST_OkButton->setEnabled (true);
    }
    else
    {
        ui->ST_OkButton->setDisabled (true);
    }
}

void securitydialog::on_ST_OkButton_clicked ()
{
    done (true);
}
