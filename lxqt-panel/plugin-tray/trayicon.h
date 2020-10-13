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


#ifndef TRAYICON_H
#define TRAYICON_H

#include <QObject>
#include <QFrame>
#include <QList>

#include <X11/X.h>
#include <X11/extensions/Xdamage.h>

#define TRAY_ICON_SIZE_DEFAULT 24

class QWidget;
class LXQtPanel;

class TrayIcon: public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QSize iconSize READ iconSize WRITE setIconSize)

public:
    TrayIcon(Window iconId, QSize const & iconSize, QWidget* parent);
    virtual ~TrayIcon();

    Window iconId() const { return mIconId; }
    Window windowId() const { return mWindowId; }
    QString appName() const { return mAppName; }

    void windowDestroyed(Window w);

    QSize iconSize() const { return mIconSize; }
    void setIconSize(QSize iconSize);

    QSize sizeHint() const;

protected:
    bool event(QEvent *event);
    void draw(QPaintEvent* event);

private:
    void init();
    QRect iconGeometry();
    Window mIconId;
    Window mWindowId;
    QString mAppName;
    QSize mIconSize;
    Damage mDamage;
    Display* mDisplay;

    static bool isXCompositeAvailable();
};

#endif // TRAYICON_H
