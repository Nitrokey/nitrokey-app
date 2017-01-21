#include <QApplication>
#include <QGridLayout>
#include <QMessageBox>
#include <QSpacerItem>
#include <QtGui>

#include "nitrokey-applet.h"


QMutex CryptostickApplet::mutex;

CryptostickApplet *csApplet(){
    return CryptostickApplet::instance();
}

void CryptostickApplet::warningBox(const QString msg) {
  QMessageBox::warning(_parent, getBrand(), msg, QMessageBox::Ok);
}

void CryptostickApplet::messageBox(const QString msg) {
//  QMessageBox msgbox(QMessageBox::Information, getBrand(), msg, QMessageBox::Ok, _parent);
//  QMessageBox::information(_parent,  getBrand(), msg, QMessageBox::Ok);
}

bool CryptostickApplet::yesOrNoBox(const QString msg, bool default_val) {
  QMessageBox::StandardButton default_btn = default_val ? QMessageBox::Yes : QMessageBox::No;

  bool b =
      QMessageBox::question(_parent, getBrand(), msg,
                            QMessageBox::Yes | QMessageBox::No, default_btn) == QMessageBox::Yes;
  return b;
}

bool CryptostickApplet::detailedYesOrNoBox(const QString msg, const QString detailed_text, bool default_val) {
  QMessageBox msgBox (QMessageBox::Question, getBrand(), msg, QMessageBox::Yes | QMessageBox::No,
                      _parent);

  msgBox.setDetailedText(detailed_text);
  // Turns out the layout box in the QMessageBox is a grid
  // You can force the resize using a spacer this way:
  QSpacerItem *horizontalSpacer =
      new QSpacerItem(400, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  QGridLayout *layout = (QGridLayout *)msgBox.layout();

  layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
  msgBox.setDefaultButton(default_val ? QMessageBox::Yes : QMessageBox::No);
  bool b = msgBox.exec() == QMessageBox::Yes;
  return b;
}

QString getBrand() {
//  const QString string = QString::fromUtf8(CRYPTOSTICK_APP_BRAND);
//  return string;
  return QStringLiteral(CRYPTOSTICK_APP_BRAND);
}

