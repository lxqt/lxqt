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

#ifndef _KBDKEEPER_H_
#define _KBDKEEPER_H_

#include <QHash>
#include <QWidget>
#include "kbdlayout.h"
#include "kbdinfo.h"
#include "settings.h"

//--------------------------------------------------------------------------------------------------

class KbdKeeper: public QObject
{
    Q_OBJECT
public:
    KbdKeeper(const KbdLayout & layout, KeeperType type = KeeperType::Global);
    virtual ~KbdKeeper();
    virtual bool setup();

    const QString & sym() const
    { return m_info.currentSym(); }

    const QString & name() const
    { return m_info.currentName(); }

    const QString & variant() const
    { return m_info.currentVariant(); }

    KeeperType type() const
    { return m_type; }

    void switchToNext();
    virtual void switchToGroup(uint group);
protected slots:
    virtual void keyboardChanged();
    virtual void layoutChanged(uint group);
    virtual void checkState();
signals:
    void changed();
protected:
    const KbdLayout & m_layout;
    KbdInfo           m_info;
    KeeperType        m_type;
};

//--------------------------------------------------------------------------------------------------

class WinKbdKeeper: public KbdKeeper
{
    Q_OBJECT
public:
    WinKbdKeeper(const KbdLayout & layout);
    virtual ~WinKbdKeeper();
    virtual void switchToGroup(uint group);
protected slots:
    virtual void layoutChanged(uint group);
    virtual void checkState();
private:
    QHash<WId, int> m_mapping;
    WId             m_active;
};

//--------------------------------------------------------------------------------------------------

class AppKbdKeeper: public KbdKeeper
{
    Q_OBJECT
public:
    AppKbdKeeper(const KbdLayout & layout);
    virtual ~AppKbdKeeper();
    virtual void switchToGroup(uint group);
protected slots:
    virtual void layoutChanged(uint group);
    virtual void checkState();
private:
    QHash<QString, int> m_mapping;
    QString             m_active;
};

#endif
