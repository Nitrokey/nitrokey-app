#include "licensedialog.h"
#include "ui_licensedialog.h"

LicenseDialog::LicenseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LicenseDialog)
{
    ui->setupUi(this);
}

LicenseDialog::~LicenseDialog()
{
    delete ui;
}


void LicenseDialog::setText(QString string)
{
    ui->textEdit->setPlainText(string);
}
