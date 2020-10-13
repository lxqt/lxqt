/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Dmitriy Zhukov <zjesclean@gmail.com>
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

#ifndef _X11KBD_H_
#define _X11KBD_H_

#include <QObject>
#include "../controls.h"

class KbdInfo;

namespace pimpl
{ class X11Kbd; }

class X11Kbd: public QObject
{
    Q_OBJECT
public:
    X11Kbd();
    virtual ~X11Kbd();

    bool init();
    bool isEnabled() const;
    void readKbdInfo(KbdInfo & info) const;
    void lockGroup(uint layId) const;
    void lockModifier(Controls cnt, bool locked);
    bool isModifierLocked(Controls cnt) const;
signals:
    void layoutChanged(uint layId);
    void modifierChanged(Controls cnt, bool locked);
    void checkState();
    void keyboardChanged();
private:
    QScopedPointer<pimpl::X11Kbd> m_priv;
};

#endif
