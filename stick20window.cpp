#include "stick20window.h"
#include "ui_stick20window.h"

Stick20Window::Stick20Window(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Stick20Window)
{
    ui->setupUi(this);
}

Stick20Window::~Stick20Window()
{
    delete ui;
}
