#include <QApplication>
#include <QGridLayout>
#include <QMessageBox>
#include <QtGui>

#include "nitrokey-applet.h"
#include "src/GUI/ManageWindow.h"

QMutex AppMessageBox::mutex;

AppMessageBox *csApplet(){
    return AppMessageBox::instance();
}

void AppMessageBox::warningBox(const QString msg) {
  QMessageBox *msgbox = new QMessageBox(
      QMessageBox::Warning, getBrand(), msg, QMessageBox::Ok, _parent);
//  msgbox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
  msgbox->setModal( true );
  ManageWindow::moveToCenter(msgbox);
  ManageWindow::bringToFocus(msgbox);
  msgbox->exec();
}

void AppMessageBox::messageBox(const QString msg) {
  QMessageBox *msgbox = new QMessageBox(
      QMessageBox::Information, getBrand(), msg, QMessageBox::Ok, _parent);
//  msgbox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
  msgbox->setModal( true );
  ManageWindow::moveToCenter(msgbox);
  ManageWindow::bringToFocus(msgbox);
  msgbox->exec();
}

bool AppMessageBox::yesOrNoBox(const QString msg, bool default_val) {
  QMessageBox::StandardButton default_btn = default_val ? QMessageBox::Yes : QMessageBox::No;

  auto buttons = QMessageBox::Yes | QMessageBox::No;
  bool b =
      QMessageBox::question(_parent, getBrand(), msg,
                            buttons, default_btn) == QMessageBox::Yes;
  //TODO make on heap
  return b;
}

bool AppMessageBox::detailedYesOrNoBox(const QString msg, const QString detailed_text, bool default_val) {
  QMessageBox *msgBox = new QMessageBox(QMessageBox::Question, getBrand(), msg, QMessageBox::Yes | QMessageBox::No,
                      _parent);

  msgBox->setDetailedText(detailed_text);
  // Turns out the layout box in the QMessageBox is a grid
  // You can force the resize using a spacer this way:
  QSpacerItem *horizontalSpacer =
      new QSpacerItem(400, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  QGridLayout *layout = (QGridLayout *)msgBox->layout();

  layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
  msgBox->setDefaultButton(default_val ? QMessageBox::Yes : QMessageBox::No);

//  msgBox->setAttribute( Qt::WA_DeleteOnClose ); //makes sure the msgbox is deleted automatically when closed
  ManageWindow::moveToCenter(msgBox);
  ManageWindow::bringToFocus(msgBox);

  bool b = msgBox->exec() == QMessageBox::Yes;
  return b;
}

QString getBrand() {
  return QStringLiteral(CRYPTOSTICK_APP_BRAND);
}

