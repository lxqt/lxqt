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

#ifndef COLORLABEL_H
#define COLORLABEL_H

#include <QLabel>
#include <QWidget>
#include <Qt>

class ColorLabel : public QLabel {
    Q_OBJECT

public:
    explicit ColorLabel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~ColorLabel();

    void setColor(const QColor& color);
    QColor getColor() const;

signals:
    void colorChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent (QPaintEvent* event) override;

private:
    QColor color_;
};

#endif // COLORLABEL_H
