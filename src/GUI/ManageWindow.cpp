#include "nitrokey-applet.h"
#include <QMessageBox>
#include <QGridLayout>
#include <QApplication>
#include "ManageWindow.h"

void ManageWindow::bringToFocus(QWidget *w) {
  w->ensurePolished();
  w->setWindowState(w->windowState() & ~Qt::WindowState::WindowMinimized);
  w->show();
  w->raise();
  w->activateWindow();
}

#include <QDesktopWidget>

void ManageWindow::moveToCenter(QWidget *widget) {
  widget->adjustSize();
  widget->move(QApplication::desktop()->screen()->rect().center() - widget->rect().center());
}