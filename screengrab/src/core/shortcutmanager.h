/***************************************************************************
 *   Copyright (C) 2009 - 2013 by Artem 'DOOMer' Galichkin                 *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QSettings>
#include <QVector>
#include <QStringList>
#include <QKeySequence>

struct Shortcut {
    QString key;
    int type;
    int action;
    Shortcut() {};
    Shortcut(QString k, int t, int a)
    {
    key = k;
    type = t;
    action = a;
    }
};

Q_DECLARE_METATYPE(Shortcut)

typedef QVector<Shortcut> ShortcutList;

class ShortcutManager
{
public:
    ShortcutManager(QSettings *settings);
    ~ShortcutManager();

    void loadSettings();
    void saveSettings();
    void setDefaultSettings();

    void setShortcut(const QString &key, int action, int type);
    QKeySequence getShortcut(int action);
    int getShortcutType(int action);
    QStringList getShortcutsList(int type);

private:
    ShortcutList _listShortcuts;
    QSettings *_shortcutSettings;
};

#endif // SHORTCUTMANAGER_H
