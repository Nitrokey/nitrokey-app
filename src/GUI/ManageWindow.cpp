#include "ManageWindow.h"

void ManageWindow::bringToFocus(QWidget *w) {
  w->ensurePolished();
  w->setWindowState(w->windowState() & ~Qt::WindowState::WindowMinimized);
  w->show();
  w->raise();
  w->activateWindow();
}
