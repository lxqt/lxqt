/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#ifndef LXQTPAGESELECTWIDGET_H
#define LXQTPAGESELECTWIDGET_H

#include "lxqtglobals.h"
#include <QListWidget>

namespace LXQt
{

class LXQT_API PageSelectWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit PageSelectWidget(QWidget *parent = nullptr);
    ~PageSelectWidget() override;
    int maxTextWidth() const;
    bool event(QEvent * event) override;

    int getWrappedTextWidth() const {
        return mWrappedTextWidth;
    }

protected:
    QSize viewportSizeHint() const override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected Q_SLOTS:
    void updateMaxTextWidth();

private:
    int mMaxTextWidth;
    int mWrappedTextWidth;
};

} // namespace LXQt
#endif // PAGESELECTWIDGET_H
