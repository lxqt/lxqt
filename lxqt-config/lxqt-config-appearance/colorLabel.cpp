/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2020 LXQt team
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

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "colorLabel.h"
#include <QColorDialog>
#include <QStyleOptionFrame>
#include <QPainter>

ColorLabel::ColorLabel(QWidget* parent, Qt::WindowFlags f)
    : QLabel(parent, f)
{
    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setLineWidth(1);
    setFixedWidth(100);
    setToolTip(tr("Click to change color."));
    color_ = palette().color(QPalette::Window);
}

ColorLabel::~ColorLabel() {}

void ColorLabel::setColor(const QColor& color)
{
    if (!color.isValid())
        return;
    color_ = color;
    color_.setAlpha(255); // ignore translucency
}

QColor ColorLabel::getColor() const
{
    return color_;
}

void ColorLabel::mousePressEvent(QMouseEvent* /*event*/)
{
    QColor prevColor = getColor();
    QColor color = QColorDialog::getColor(prevColor, window(), tr("Select Color"));
    if (color.isValid() && color != prevColor)
    {
        emit colorChanged();
        setColor(color);
    }
}

void ColorLabel::paintEvent (QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.fillRect(contentsRect(), color_);
    QStyleOptionFrame opt;
    initStyleOption(&opt);
    style()->drawControl(QStyle::CE_ShapedFrame, &opt, &p, this);
}

