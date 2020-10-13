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

#include "shortcut_selector.h"
#include "actions.h"

#include <QTimer>
#include <QMenu>


ShortcutSelector::ShortcutSelector(Actions *actions, QWidget *parent)
    : QToolButton(parent)
    , mActions(nullptr)
    , mShortcutTimer(new QTimer(this))
    , mAutoApplyShortcut(false)
{
    init();
    setActions(actions);
}

ShortcutSelector::ShortcutSelector(QWidget *parent)
    : QToolButton(parent)
    , mActions(nullptr)
    , mShortcutTimer(new QTimer(this))
    , mAutoApplyShortcut(false)
{
    init();
}

void ShortcutSelector::init()
{
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);

    mShortcutTimer->setInterval(1000);
    mShortcutTimer->setSingleShot(false);

    connect(this, SIGNAL(clicked()), this, SLOT(grabShortcut()));

    connect(mShortcutTimer, SIGNAL(timeout()), this, SLOT(shortcutTimer_timeout()));
}

void ShortcutSelector::setActions(Actions *actions)
{
    if (mActions)
    {
        return;
    }
    mActions = actions;
    connect(mActions, SIGNAL(grabShortcutCancelled()), this, SLOT(grabShortcut_fail()));
    connect(mActions, SIGNAL(grabShortcutTimedout()), this, SLOT(grabShortcut_fail()));
    connect(mActions, SIGNAL(grabShortcutFailed()), this, SLOT(grabShortcut_fail()));
    connect(mActions, SIGNAL(shortcutGrabbed(QString)), this, SLOT(newShortcutGrabbed(QString)));
}

void ShortcutSelector::grabShortcut(int timeout)
{
    if (!mActions)
    {
        return;
    }

    if (!isChecked())
    {
        mActions->cancelShortcutGrab();
        return;
    }

    mTimeoutCounter = timeout;
    mOldShortcut = text();
    setText(QString::number(mTimeoutCounter));
    mShortcutTimer->start();
    mActions->grabShortcut(mTimeoutCounter * mShortcutTimer->interval());
}

void ShortcutSelector::shortcutTimer_timeout()
{
    if (mTimeoutCounter > 0)
    {
        --mTimeoutCounter;
        setText(QString::number(mTimeoutCounter));
    }
    else
        setText(QString());
}

void ShortcutSelector::grabShortcut_fail()
{
    if (isChecked())
    {
        setChecked(false);
        mShortcutTimer->stop();
        setText(mOldShortcut);
        emit shortcutGrabbed(mOldShortcut);
    }
}

void ShortcutSelector::newShortcutGrabbed(const QString &newShortcut)
{
    setChecked(false);
    mShortcutTimer->stop();
    if (mAutoApplyShortcut)
        setText(newShortcut);
    else
        setText(QString());
    emit shortcutGrabbed(newShortcut);
}

void ShortcutSelector::clear()
{
    setText(QString());
}

QAction * ShortcutSelector::addMenuAction(const QString &title)
{
    QMenu *subMenu = menu();
    if (!subMenu)
    {
        setPopupMode(QToolButton::MenuButtonPopup);
        subMenu = new QMenu(this);
        setMenu(subMenu);
    }
    QAction *action = new QAction(title, subMenu);
    subMenu->addAction(action);
    return action;
}

bool ShortcutSelector::isGrabbing() const
{
    return isChecked();
}

void ShortcutSelector::cancelNow()
{
    if (isChecked())
    {
        grabShortcut_fail();
        mActions->cancelShortcutGrab();
    }
}
