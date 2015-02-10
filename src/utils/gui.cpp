/*
TRANSLATOR Gui::ColorButton
*/
/*
 * Wally - Qt4 wallpaper/background changer
 * Copyright (C) 2009  Antonio Di Monaco <tony@becrux.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QStylePainter>
#include <QColorDialog>
#include <QPushButton>
#include <QButtonGroup>

#include "gui.h"

using namespace Gui;

EngineButton::EngineButton(QWidget *parent) : QPushButton(parent)
{
  setup();
}

EngineButton::EngineButton(const QString &text, QWidget *parent) :
  QPushButton(text,parent)
{
  setup();
}

EngineButton::EngineButton(const QIcon &icon, const QString &text, QWidget *parent) :
  QPushButton(icon,text,parent)
{
  setup();
}

QSize EngineButton::sizeHint() const
{
  if (_pixmap.isNull())
    return QSize(QPushButton::sizeHint().width(),(_height == -1)? QPushButton::sizeHint().height() : _height);
  else
    return QSize(_pixmap.width() + 10,(_height == -1)? _pixmap.height() + 10 : _height);
}

void EngineButton::setPixmap(const QPixmap &pixmap)
{
  _pixmap = pixmap;
  update();
}

void EngineButton::paintEvent(QPaintEvent * /* event */)
{
  QStyleOptionButton optionButton;
  QStylePainter painter(this);

  initStyleOption(&optionButton);

  if (_isOver && !isDown())
  {
    optionButton.features |= QStyleOptionButton::None;
    optionButton.features &= ~QStyleOptionButton::Flat;
    optionButton.state |= QStyle::State_Raised;
    optionButton.state &= ~QStyle::State_Sunken;
  }
  else
  {
    optionButton.features |= QStyleOptionButton::Flat;
    optionButton.features &= ~QStyleOptionButton::None;
  }

  optionButton.state &= ~QStyle::State_HasFocus;

  painter.drawControl(QStyle::CE_PushButton,optionButton);

  if (!_pixmap.isNull())
    painter.drawPixmap((width() - _pixmap.width()) / 2,
                       (height() - _pixmap.height()) / 2,_pixmap);
}

void EngineButton::enterEvent(QEvent * /* event */)
{
  _isOver = true;
  repaint();
}

void EngineButton::leaveEvent(QEvent * /* event */)
{
  _isOver = false;
  repaint();
}

void EngineButton::setup()
{
  QString newText = text().replace(" ","\n");
  QFont newFont(font());

  _height = -1;
  _isOver = false;
  newFont.setPointSize(13);

  setText(newText);
  setFlat(true);
  setCheckable(true);
  setFont(newFont);
}

void ColorButton::setColor(const QColor &color)
{
  if (color.isValid())
  {
    _color = color;
    repaint();
    emit colorChanged(_color);
  }
}

void ColorButton::paintEvent(QPaintEvent *event)
{
  QPushButton::paintEvent(event);

  QPainter painter(this);
  QStyleOptionButton option;

  option.initFrom(this);
  option.text = (_color.alpha())? QString() : tr("Auto");
  painter.setRenderHint(QPainter::Antialiasing,true);

  QRect rect = QApplication::style()->subElementRect(QStyle::SE_PushButtonContents,&option,this);

  if (_color.alpha())
    painter.fillRect(rect.adjusted(rect.width() / 8,rect.height() / 3,-rect.width() / 8,-rect.height() / 3),color());
  else
    painter.drawText(rect,Qt::AlignCenter,tr("Auto"));
}

void ColorButton::nextCheckState()
{
  QPushButton::nextCheckState();

  if (color().alpha())
    setColor(QColor(color().red(),color().green(),color().blue(),0));
  else
  {
    QColor newColor = QColorDialog::getColor(color().rgb());
    if (newColor.isValid())
      setColor(newColor);
  }
}

void Dialog::showEvent(QShowEvent *event)
{
  if (!fired)
    emit executed();

  QDialog::showEvent(event);

  if (!fired)
  {
    fired = true;

    updateGeometry();

    move((QApplication::desktop()->availableGeometry().width() - width()) / 2,
         (_position == CenterOfScreen)?
           (QApplication::desktop()->availableGeometry().height() - height()) / 2 :
           QApplication::desktop()->availableGeometry().height() / 10);
  }
}

void Dialog::closeEvent(QCloseEvent *event)
{
  emit closed();
  QDialog::closeEvent(event);
}

ScrollableButtons::ScrollableButtons(QWidget *parent) : QFrame(parent)
{
  QPalette appPalette = QApplication::palette();
  QWidget *buttonWidget = new QWidget(this);
  QVBoxLayout *layout = new QVBoxLayout;

  firstShowEvent = true;
  setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);

  buttonUp = new QPushButton(QIcon(":/images/up"),QString(),this);
  buttonDown = new QPushButton(QIcon(":/images/down"),QString(),this);

  appPalette.setColor(QPalette::Window,appPalette.color(QPalette::Base));
  setPalette(appPalette);

  buttonUp->setAutoRepeat(true);
  buttonDown->setAutoRepeat(true);

  scrollArea = new QScrollArea(this);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setFrameShape(QFrame::NoFrame);
  scrollArea->verticalScrollBar()->setTracking(true);
  scrollArea->setContentsMargins(0,0,0,0);

  buttonLayout = new QVBoxLayout;
  buttonLayout->setSpacing(0);
  buttonLayout->setContentsMargins(0,0,0,0);
  buttonWidget->setLayout(buttonLayout);
  buttonWidget->setContentsMargins(0,0,0,0);
  scrollArea->setWidget(buttonWidget);
  scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  scrollArea->setWidgetResizable(true);

  layout->addWidget(buttonUp);
  layout->addWidget(scrollArea);
  layout->addWidget(buttonDown);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(3);

  setLayout(layout);

  setFrameShape(QFrame::StyledPanel);
  setFrameShadow(QFrame::Plain);

  connect(buttonUp,SIGNAL(clicked()),this,SLOT(on_buttonUp_clicked()));
  connect(buttonDown,SIGNAL(clicked()),this,SLOT(on_buttonDown_clicked()));
  connect(scrollArea->verticalScrollBar(),SIGNAL(valueChanged(int)),
          this,SLOT(on_scrollBar_valueChanged(int)));

  buttonGroup = new QButtonGroup(this);
  connect(buttonGroup,SIGNAL(buttonClicked(QAbstractButton *)),
          this,SIGNAL(buttonClicked(QAbstractButton *)));
  connect(buttonGroup,SIGNAL(buttonClicked(int)),
          this,SIGNAL(buttonClicked(int)));
  connect(buttonGroup,SIGNAL(buttonPressed(QAbstractButton *)),
          this,SIGNAL(buttonPressed(QAbstractButton *)));
  connect(buttonGroup,SIGNAL(buttonPressed(int)),
          this,SIGNAL(buttonPressed(int)));
  connect(buttonGroup,SIGNAL(buttonReleased(QAbstractButton *)),
          this,SIGNAL(buttonReleased(QAbstractButton *)));
  connect(buttonGroup,SIGNAL(buttonReleased(int)),
          this,SIGNAL(buttonReleased(int)));
}

void ScrollableButtons::on_buttonUp_clicked()
{
  scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() - 10);
}

void ScrollableButtons::on_buttonDown_clicked()
{
  scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() + 10);
}

void ScrollableButtons::addButton(QAbstractButton *button)
{
  buttonLayout->addWidget(button);
  buttonGroup->addButton(button);
}

void ScrollableButtons::addButton(QAbstractButton *button, int id)
{
  buttonLayout->addWidget(button);
  buttonGroup->addButton(button,id);
}

QAbstractButton *ScrollableButtons::button(int id) const
{
  return buttonGroup->button(id);
}

QList<QAbstractButton *> ScrollableButtons::buttons() const
{
  return buttonGroup->buttons();
}

QAbstractButton *ScrollableButtons::checkedButton() const
{
  return buttonGroup->checkedButton();
}

int ScrollableButtons::checkedId() const
{
  return buttonGroup->checkedId();
}

bool ScrollableButtons::exclusive() const
{
  return buttonGroup->exclusive();
}

int ScrollableButtons::id(QAbstractButton *button) const
{
  return buttonGroup->id(button);
}

void ScrollableButtons::insertButton(int index, QAbstractButton *button)
{
  buttonLayout->insertWidget(index,button);
  buttonGroup->addButton(button);
}

void ScrollableButtons::insertButton(int index, QAbstractButton *button, int id)
{
  buttonLayout->insertWidget(index,button);
  buttonGroup->addButton(button,id);
}

void ScrollableButtons::removeButton(QAbstractButton *button)
{
  buttonLayout->removeWidget(button);
  buttonGroup->removeButton(button);
}

void ScrollableButtons::setExclusive(bool exclusive)
{
  buttonGroup->setExclusive(exclusive);
}

void ScrollableButtons::setId(QAbstractButton *button, int id)
{
  buttonGroup->setId(button,id);
}

void ScrollableButtons::on_scrollBar_valueChanged(int value)
{
  buttonUp->setEnabled(value > scrollArea->verticalScrollBar()->minimum());
  buttonDown->setEnabled(value < scrollArea->verticalScrollBar()->maximum());
}

void ScrollableButtons::resizeEvent(QResizeEvent *event)
{
  QFrame::resizeEvent(event);
  QTimer::singleShot(0,this,SLOT(scrollToSelected()));
}

void ScrollableButtons::showEvent(QShowEvent *event)
{
  qDebug() << "ScrollableButtons::showEvent";

  QFrame::showEvent(event);
  if (firstShowEvent)
  {
    firstShowEvent = false;
    QTimer::singleShot(0,this,SLOT(scrollToSelected()));
  }
}

void ScrollableButtons::scrollToSelected()
{
  QAbstractButton *selectedButton = 0;
  QListIterator<QAbstractButton *> button(buttonGroup->buttons());

  while (button.hasNext())
  {
    QAbstractButton *buttonItem = button.next();

    if (buttonItem->isChecked())
      selectedButton = buttonItem;
  }

  if (selectedButton)
    scrollToButton(selectedButton);
}

void ShadowedLabel::paintEvent(QPaintEvent * /* event */)
{
  int delta = (font().pointSize() > 10)? 2 : 1;
  QPainter painter(this);

  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);

  painter.setPen(_shadowColor);
  painter.setBrush(_shadowColor);
  painter.drawText(rect().adjusted(0,0,-delta,-delta).translated(delta,delta),alignment(),text());

  painter.setPen(_labelColor);
  painter.setBrush(_labelColor);
  painter.drawText(rect().adjusted(0,0,-delta,-delta),alignment(),text());
}

QSize ShadowedLabel::sizeHint() const
{
  return QLabel::sizeHint() + ((font().pointSize() > 10)? QSize(2,2) : QSize(1,1));
}

ShrinkableTableWidget::ShrinkableTableWidget(QWidget *parent) : QTableWidget(parent)
{
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
}

ShrinkableTableWidget::ShrinkableTableWidget(int rows, int columns, QWidget *parent) : QTableWidget(rows,columns,parent)
{
  setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
}

QSize ShrinkableTableWidget::sizeHint() const
{
  int row, height;

  for (row = height = 0; row < rowCount(); ++row)
    height += rowHeight(row);

  return QSize(QTableWidget::sizeHint().width(),height + 10);
}
