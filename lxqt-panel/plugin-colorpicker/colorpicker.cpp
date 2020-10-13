/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Aaron Lewis <the.warl0ck.1989@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "colorpicker.h"
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QScreen>


ColorPicker::ColorPicker(const ILXQtPanelPluginStartupInfo &startupInfo) :
    QObject(),
    ILXQtPanelPlugin(startupInfo)
{
    realign();
}


ColorPicker::~ColorPicker()
{
}


void ColorPicker::realign()
{
    mWidget.button()->setFixedHeight(panel()->iconSize());
    mWidget.button()->setFixedWidth(panel()->iconSize());
    mWidget.lineEdit()->setFixedHeight(panel()->iconSize());
}


ColorPickerWidget::ColorPickerWidget(QWidget *parent):
    QFrame(parent)
{
    QFontMetrics fm (mLineEdit.font());
    mLineEdit.setFixedWidth ( 10*fm.width (QStringLiteral("a")) );

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins (0, 0, 0, 0);
    layout->setSpacing (1);
    setLayout(layout);
    layout->addWidget (&mButton);
    layout->addWidget (&mLineEdit);


    mButton.setAutoRaise(true);
    mButton.setIcon(XdgIcon::fromTheme(QStringLiteral("color-picker"), QStringLiteral("kcolorchooser")));

    mCapturing = false;
    connect(&mButton, SIGNAL(clicked()), this, SLOT(captureMouse()));

}


ColorPickerWidget::~ColorPickerWidget()
{
}


void ColorPickerWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!mCapturing)
        return;

    WId id = QApplication::desktop()->winId();
    QPixmap pixmap = qApp->primaryScreen()->grabWindow(id, event->globalX(), event->globalY(), 1, 1);

    QImage img = pixmap.toImage();
    QColor col = QColor(img.pixel(0,0));

    mLineEdit.setText (col.name());

    mCapturing = false;
    releaseMouse();
}


void ColorPickerWidget::captureMouse()
{
    grabMouse(Qt::CrossCursor);
    mCapturing = true;
}
