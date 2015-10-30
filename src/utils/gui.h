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

#ifndef GUI_H
#define GUI_H

#include <QtCore>
#include <QtGui>
#include <QWidget>
#include <QPushButton>
#include <QDialog>
#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QLabel>
#include <QTableWidget>


namespace Gui
{
    class EngineButton:public QPushButton
    {
        Q_OBJECT int _height;
        QPixmap _pixmap;
        bool _isOver;
        bool _isActive;

        void setup ();

      protected:
        void paintEvent (QPaintEvent * event);
        void enterEvent (QEvent * event);
        void leaveEvent (QEvent * event);

      public:
          EngineButton (QWidget * parent = 0);
          EngineButton (const QString & text, QWidget * parent = 0);
          EngineButton (const QIcon & icon, const QString & text, QWidget * parent = 0);
          virtual ~ EngineButton ()
        {
        }

        void setHeight (int height)
        {
            _height = height;
            updateGeometry ();
        }
        void setPixmap (const QPixmap & pixmap);

        QSize sizeHint () const;
    };

    class ColorButton:public QPushButton
    {
        Q_OBJECT QColor _color;

      protected:
        void paintEvent (QPaintEvent * event);
        void nextCheckState ();

      public:
        ColorButton (QWidget * parent = 0):QPushButton (parent), _color (Qt::black)
        {
        }
      ColorButton (const QColor & color, QWidget * parent = 0):QPushButton (parent), _color (color)
        {
        }
        virtual ~ ColorButton ()
        {
        }

        void setColor (const QColor & color);

        QColor color () const
        {
            return _color;
        }

        QSize sizeHint () const
        {
            return QSize (90, QPushButton::sizeHint ().height ());
        }

        signals:void colorChanged (const QColor & color);
    };

    class Dialog:public QDialog
    {
      Q_OBJECT public:
        enum Position
        { CenterOfScreen, TopOfScreen };

      private:
          bool fired;
        Position _position;

      protected:
        void showEvent (QShowEvent * event);
        void closeEvent (QCloseEvent * event);

      public:
        Dialog (Position position, QWidget * parent = 0, Qt::WindowFlags f = 0):
        QDialog (parent, f | Qt::WindowSystemMenuHint), fired (false), _position (position)
        {
            setWindowModality (Qt::ApplicationModal);
            setMaximumWidth (QApplication::desktop ()->availableGeometry ().width () * 9 / 10);
            setMaximumHeight (QApplication::desktop ()->availableGeometry ().height () * 8 / 10);
        }
        virtual ~ Dialog ()
        {
        }

      signals:
        void executed ();

        void closed ();
    };

    class ScrollableButtons:public QFrame
    {
        Q_OBJECT bool firstShowEvent;
        QPushButton* buttonUp;
        QPushButton* buttonDown;
        QScrollArea* scrollArea;
        QVBoxLayout* buttonLayout;
        QButtonGroup* buttonGroup;

        private slots:void on_buttonUp_clicked ();
        void on_buttonDown_clicked ();
        void on_scrollBar_valueChanged (int value);

      protected:
        void resizeEvent (QResizeEvent * event);
        void showEvent (QShowEvent * event);

      public:
          ScrollableButtons (QWidget * parent = 0);
          virtual ~ ScrollableButtons ()
        {
        }

        void addButton (QAbstractButton * button);

        void addButton (QAbstractButton * button, int id);

        QAbstractButton* button (int id) const;

        QList < QAbstractButton * >buttons ()const;

        QAbstractButton* checkedButton () const;

        int checkedId () const;

        bool exclusive () const;

        int id (QAbstractButton * button) const;

        void insertButton (int index, QAbstractButton * button);

        void insertButton (int index, QAbstractButton * button, int id);

        void removeButton (QAbstractButton * button);

        void setExclusive (bool exclusive);

        void setId (QAbstractButton * button, int id);

      signals:
        void buttonClicked (QAbstractButton * button);

        void buttonClicked (int id);

        void buttonPressed (QAbstractButton * button);

        void buttonPressed (int id);

        void buttonReleased (QAbstractButton * button);

        void buttonReleased (int id);

        public slots:void scrollAllUp ()
        {
            scrollArea->verticalScrollBar ()->setValue (scrollArea->verticalScrollBar ()->minimum ());
        }
        void scrollAllDown ()
        {
            scrollArea->verticalScrollBar ()->setValue (scrollArea->verticalScrollBar ()->maximum ());
        }
        void scrollToButton (QAbstractButton * button)
        {
            scrollArea->ensureWidgetVisible (button);
        }
        void scrollToSelected ();
    };

    class ShadowedLabel:public QLabel
    {
        QColor _labelColor;
        QColor _shadowColor;

      protected:
        void paintEvent (QPaintEvent * event);

      public:
        ShadowedLabel (QWidget * parent = 0):
        QLabel (parent), _labelColor (Qt::white), _shadowColor (Qt::black)
        {
        }
      ShadowedLabel (const QString & text, QWidget * parent = 0):
        QLabel (text, parent), _labelColor (Qt::white), _shadowColor (Qt::black)
        {
        }
        virtual ~ ShadowedLabel ()
        {
        }

        void setLabelColor (const QColor & color);

        void setShadowColor (const QColor & color);

        QColor labelColor () const
        {
            return _labelColor;
        }
        QColor shadowColor () const
        {
            return _shadowColor;
        }

        QSize sizeHint () const;
    };

    class ShrinkableTableWidget:public QTableWidget
    {
      Q_OBJECT public:
          ShrinkableTableWidget (QWidget * parent = 0);
          ShrinkableTableWidget (int rows, int columns, QWidget * parent = 0);
          virtual ~ ShrinkableTableWidget ()
        {
        }

        QSize sizeHint () const;
    };
}

#endif
