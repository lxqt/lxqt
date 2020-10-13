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

#ifndef GLOBAL_KEY_SHORTCUT_CLIENT__ACTION__IMPL__INCLUDED
#define GLOBAL_KEY_SHORTCUT_CLIENT__ACTION__IMPL__INCLUDED

#include <QObject>
#include <QString>
#include <QMap>
#include <QSharedPointer>


namespace GlobalKeyShortcut
{

class Action;
class ClientImpl;

class ActionImpl : public QObject
{
    Q_OBJECT

public:
    ActionImpl(ClientImpl *client, Action *interface, const QString &path, const QString &description, QObject *parent = nullptr);
    ~ActionImpl() override;

    QString changeShortcut(const QString &shortcut);
    bool changeDescription(const QString &description);

    void setShortcut(const QString &shortcut);

    QString path() const;
    QString shortcut() const;
    QString description() const;

    void setValid(bool valid);
    bool isValid() const;

    void setRegistrationPending(bool registrationPending);
    bool isRegistrationPending() const;

public slots:
    void activated();
    void shortcutChanged(const QString &oldShortcut, const QString &newShortcut);

signals:
    void emitRegistrationFinished();
    void emitActivated();
    void emitShortcutChanged(const QString &oldShortcut, const QString &newShortcut);

private:
    ClientImpl *mClient;
    Action *mInterface;
    QString mAlias;
    QString mPath;
    QString mShortcut;
    QString mDescription;
    bool mValid;
    bool mRegistrationPending;
};

}

#endif // GLOBAL_KEY_SHORTCUT_CLIENT__ACTION__IMPL__INCLUDED
