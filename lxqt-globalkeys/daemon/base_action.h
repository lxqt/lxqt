/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#ifndef GLOBAL_ACTION_DAEMON__BASE_ACTION__INCLUDED
#define GLOBAL_ACTION_DAEMON__BASE_ACTION__INCLUDED


#include <QString>

class LogTarget;

class BaseAction
{
public:
    BaseAction(LogTarget *logTarget, const QString &description);
    virtual ~BaseAction();

    virtual const char *type() const = 0;

    virtual bool call() = 0;

    const QString &description() const { return mDescription; }
    void setDescription(const QString &description) { mDescription = description; }

    void setEnabled(bool value = true) { mEnabled = value; }
    void setDisabled(bool value = true) { mEnabled = !value; }
    bool isEnabled() const { return mEnabled; }

protected:
    LogTarget *mLogTarget;

private:
    QString mDescription;

    bool mEnabled;
};

#endif // GLOBAL_ACTION_DAEMON__BASE_ACTION__INCLUDED
