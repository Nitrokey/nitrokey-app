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

#include "gui.h"
#include "splash.h"

#define TITLE_FONT_POINT_SIZE 40
#define VERSION_FONT_POINT_SIZE 5
#define TEXT_MARGIN 10

SplashScreen::SplashScreen ():QSplashScreen (QPixmap (),
               Qt::WindowStaysOnTopHint)
{
QFont titleFont = qApp->font ();

QFont authorFont = qApp->font ();

QFont versionFont = qApp->font ();

    logo = new QPixmap (":/images/splash.png");

QWidget* textWidget = new QWidget (this);

QVBoxLayout* textLayout = new QVBoxLayout;

    Gui::ShadowedLabel * titleLabel =
        new Gui::ShadowedLabel (qApp->applicationName (), this);
    titleLabel->setAlignment (Qt::AlignRight | Qt::AlignVCenter);
    titleFont.setBold (true);
    titleFont.setPointSize (40);
    titleLabel->setFont (titleFont);
    Gui::ShadowedLabel * versionLabel =
        new Gui::ShadowedLabel (qApp->applicationVersion (), this);
    versionFont.setPointSize (20);
    versionLabel->setFont (versionFont);
    versionLabel->setAlignment (Qt::AlignRight | Qt::AlignVCenter);
    Gui::ShadowedLabel * authorLabel =
        new Gui::ShadowedLabel (tr ("Author:") + "", this);
    authorFont.setPointSize (10);
    authorLabel->setFont (authorFont);
    authorLabel->setAlignment (Qt::AlignRight | Qt::AlignVCenter);
QWidget* spacerWidget = new QWidget (this);

    spacerWidget->setFixedHeight (15);

    textLayout->setSpacing (1);
    textLayout->addStretch ();
    textLayout->addWidget (titleLabel);
    textLayout->addWidget (versionLabel);
    textLayout->addWidget (spacerWidget);
    textLayout->addWidget (authorLabel);

    textWidget->setLayout (textLayout);

    textWidget->setFixedSize (logo->size ().width (),
                              logo->height () -
                              QFontMetrics (qApp->font ()).height () - 12);

QPainter painter (logo);

    painter.setRenderHint (QPainter::Antialiasing, true);
    painter.setRenderHint (QPainter::TextAntialiasing, true);
    painter.
        fillRect (QRect
                  (8,
                   logo->height () - QFontMetrics (qApp->font ()).height () -
                   12, logo->width (), logo->height ()), Qt::black);
    painter.end ();
    textWidget->render (logo, QPoint (), QRegion (), 0);

    setMask (QRegion (logo->mask ()));
    setPixmap (*logo);
}

SplashScreen::~SplashScreen ()
{
delete logo;
}
