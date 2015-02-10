#ifndef STICK20WINDOW_H
#define STICK20WINDOW_H

#include <QMainWindow>

namespace Ui {
class Stick20Window;
}

class Stick20Window : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit Stick20Window(QWidget *parent = 0);
    ~Stick20Window();
    
private:
    Ui::Stick20Window *ui;
};

#endif // STICK20WINDOW_H
