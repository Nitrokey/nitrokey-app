#ifndef NITROKEYAPP_MANAGEWINDOW_H
#define NITROKEYAPP_MANAGEWINDOW_H


#include <QWidget>

class ManageWindow {
public:
  static void bringToFocus(QWidget * w);
  static void moveToCenter(QWidget *widget);
};


#endif //NITROKEYAPP_MANAGEWINDOW_H
