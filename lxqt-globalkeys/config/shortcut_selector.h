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

#ifndef GLOBAL_ACTION_CONFIG__SHORTCUT_SELECTOR__INCLUDED
#define GLOBAL_ACTION_CONFIG__SHORTCUT_SELECTOR__INCLUDED

#include <QToolButton>
#include <QWidget>
#include <QString>


class Actions;
class QTimer;

class ShortcutSelector : public QToolButton
{
    Q_OBJECT
public:
    explicit ShortcutSelector(Actions *actions, QWidget *parent = nullptr);
    explicit ShortcutSelector(QWidget *parent = nullptr);
    void setActions(Actions *actions);

    QAction *addMenuAction(const QString &title);

    bool shortcutAutoApplied(void) const { return mAutoApplyShortcut; }

    bool isGrabbing() const;

signals:
    void shortcutGrabbed(const QString &);

public slots:
    void grabShortcut(int timeout = 10);

    void clear();

    void autoApplyShortcut(bool value = true) { mAutoApplyShortcut = value; }

    void cancelNow();

private slots:
    void shortcutTimer_timeout();
    void grabShortcut_fail();
    void newShortcutGrabbed(const QString &);

private:
    Actions *mActions;
    QString mOldShortcut;
    int mTimeoutCounter;
    QTimer *mShortcutTimer;
    bool mAutoApplyShortcut;

    void init();
};

#endif // GLOBAL_ACTION_CONFIG__SHORTCUT_SELECTOR__INCLUDED
