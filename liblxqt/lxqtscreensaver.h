/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
 *
 * Copyright (c) 2016 Luís Pereira <luis.artur.pereira@gmail.com>
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

#ifndef SCREENSAVER_H
#define SCREENSAVER_H

#include "lxqtglobals.h"
#include <QObject>
#include <QAction>

namespace LXQt
{

class ScreenSaverPrivate;

class LXQT_API ScreenSaver : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ScreenSaver)
    Q_DISABLE_COPY(ScreenSaver)

public:
    ScreenSaver(QObject * parent=nullptr);
    ~ScreenSaver() override;

    QList<QAction*> availableActions();

Q_SIGNALS:
    void activated();
    void done();
public Q_SLOTS:
    void lockScreen();

private:
    ScreenSaverPrivate* const d_ptr;
};

} // namespace LXQt
#endif

