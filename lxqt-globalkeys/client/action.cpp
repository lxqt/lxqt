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

#include "action.h"
#include "action_p.h"
#include "client_p.h"

#include "org.lxqt.global_key_shortcuts.client.h"


namespace GlobalKeyShortcut
{

ActionImpl::ActionImpl(ClientImpl *client, Action *interface, const QString &path, const QString &description, QObject *parent)
    : QObject(parent)
    , mClient(client)
    , mInterface(interface)
    , mPath(path)
    , mDescription(description)
    , mRegistrationPending(false)
{
    new OrgLxqtActionClientAdaptor(this);

    connect(this, SIGNAL(emitRegistrationFinished()), mInterface, SIGNAL(registrationFinished()));
    connect(this, SIGNAL(emitActivated()), mInterface, SIGNAL(activated()));
    connect(this, SIGNAL(emitShortcutChanged(QString, QString)), mInterface, SIGNAL(shortcutChanged(QString, QString)));
}

ActionImpl::~ActionImpl()
{
    mClient->removeAction(this);
}

QString ActionImpl::changeShortcut(const QString &shortcut)
{
    if (mRegistrationPending)
        return mShortcut;

    mShortcut = mClient->changeClientActionShortcut(mPath, shortcut);
    return mShortcut;
}

bool ActionImpl::changeDescription(const QString &description)
{
    if (mRegistrationPending)
        return false;

    bool result = mClient->modifyClientAction(mPath, description);
    if (result)
    {
        mDescription = description;
    }
    return result;
}

void ActionImpl::setShortcut(const QString &shortcut)
{
    mShortcut = shortcut;
}

QString ActionImpl::path() const
{
    return mPath;
}

QString ActionImpl::shortcut() const
{
    return mShortcut;
}

QString ActionImpl::description() const
{
    return mDescription;
}

void ActionImpl::setValid(bool valid)
{
    mValid = valid;
}

bool ActionImpl::isValid() const
{
    return mValid;
}

void ActionImpl::setRegistrationPending(bool registrationPending)
{
    mRegistrationPending = registrationPending;
    if (!mRegistrationPending)
        emit emitRegistrationFinished();

}

bool ActionImpl::isRegistrationPending() const
{
    return mRegistrationPending;
}

void ActionImpl::activated()
{
    emit emitActivated();
}

void ActionImpl::shortcutChanged(const QString &oldShortcut, const QString &newShortcut)
{
    emit emitShortcutChanged(oldShortcut, newShortcut);
}


Action::Action(QObject *parent)
    : QObject(parent)
    , impl(nullptr)
{
}

Action::~Action()
{
}

QString Action::changeShortcut(const QString &shortcut) { return impl->changeShortcut(shortcut); }
bool Action::changeDescription(const QString &description) { return impl->changeDescription(description); }
QString Action::path() const { return impl->path(); }
QString Action::shortcut() const { return impl->shortcut(); }
QString Action::description() const { return impl->description(); }
bool Action::isValid() const { return impl->isValid(); }

}
