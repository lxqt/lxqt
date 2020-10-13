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

#include "shortcutmanager.h"
#include "src/core/config.h"

const QString DEF_SHORTCUT_NEW = QStringLiteral("Ctrl+N");
const QString DEF_SHORTCUT_SAVE = QStringLiteral("Ctrl+S");
const QString DEF_SHORTCUT_COPY = QStringLiteral("Ctrl+C");
const QString DEF_SHORTCUT_OPT = QStringLiteral("Ctrl+P");
const QString DEF_SHORTCUT_HELP = QStringLiteral("F1");
const QString DEF_SHORTCUT_CLOSE = QStringLiteral("Esc");
const QString DEF_SHORTCUT_FULL = QLatin1String("");
const QString DEF_SHORTCUT_ACTW = QLatin1String("");
const QString DEF_SHORTCUT_AREA = QLatin1String("");

const QString KEY_SHORTCUT_FULL = QStringLiteral("FullScreen");
const QString KEY_SHORTCUT_ACTW = QStringLiteral("ActiveWindow");
const QString KEY_SHORTCUT_AREA = QStringLiteral("AreaSelection");
const QString KEY_SHORTCUT_NEW = QStringLiteral("NewScreen");
const QString KEY_SHORTCUT_SAVE = QStringLiteral("SaveScreen");
const QString KEY_SHORTCUT_COPY = QStringLiteral("CopyScreen");
const QString KEY_SHORTCUT_OPT = QStringLiteral("Options");
const QString KEY_SHORTCUT_HELP = QStringLiteral("Help");
const QString KEY_SHORTCUT_CLOSE = QStringLiteral("Close");

ShortcutManager::ShortcutManager(QSettings *settings) :
    _shortcutSettings(new QSettings)
{
    _shortcutSettings = settings;

    for (int i = Config::shortcutFullScreen; i <= Config::shortcutClose; ++i)
        _listShortcuts << Shortcut();
}

ShortcutManager::~ShortcutManager()
{
    _shortcutSettings = nullptr;
    delete _shortcutSettings;
}

void ShortcutManager::loadSettings()
{
    _shortcutSettings->beginGroup(QStringLiteral("LocalShortcuts"));
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_NEW, DEF_SHORTCUT_NEW).toString(),
                Config::shortcutNew, Config::localShortcut);
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_SAVE, DEF_SHORTCUT_SAVE).toString(),
                Config::shortcutSave, Config::localShortcut);
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_COPY, DEF_SHORTCUT_COPY).toString(),
                Config::shortcutCopy, Config::localShortcut);
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_OPT, DEF_SHORTCUT_OPT).toString(),
                Config::shortcutOptions, Config::localShortcut);
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_HELP, DEF_SHORTCUT_HELP).toString(),
                Config::shortcutHelp, Config::localShortcut);
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_CLOSE, DEF_SHORTCUT_CLOSE).toString(),
                Config::shortcutClose, Config::localShortcut);
    _shortcutSettings->endGroup();

    _shortcutSettings->beginGroup(QStringLiteral("GlobalShortcuts"));
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_FULL, DEF_SHORTCUT_FULL).toString(),
                Config::shortcutFullScreen, Config::globalShortcut);
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_ACTW, DEF_SHORTCUT_ACTW).toString(),
                Config::shortcutActiveWnd, Config::globalShortcut);
    setShortcut(_shortcutSettings->value(KEY_SHORTCUT_AREA, DEF_SHORTCUT_AREA).toString(),
                Config::shortcutAreaSelect, Config::globalShortcut);
    _shortcutSettings->endGroup();
}

void ShortcutManager::saveSettings()
{
    _shortcutSettings->beginGroup(QStringLiteral("LocalShortcuts"));
    _shortcutSettings->setValue(KEY_SHORTCUT_NEW, getShortcut(Config::shortcutNew));
    _shortcutSettings->setValue(KEY_SHORTCUT_SAVE, getShortcut(Config::shortcutSave));
    _shortcutSettings->setValue(KEY_SHORTCUT_COPY, getShortcut(Config::shortcutCopy));
    _shortcutSettings->setValue(KEY_SHORTCUT_OPT, getShortcut(Config::shortcutOptions));
    _shortcutSettings->setValue(KEY_SHORTCUT_HELP, getShortcut(Config::shortcutHelp));
    _shortcutSettings->setValue(KEY_SHORTCUT_CLOSE, getShortcut(Config::shortcutClose));
    _shortcutSettings->endGroup();

    _shortcutSettings->beginGroup(QStringLiteral("GlobalShortcuts"));
    _shortcutSettings->setValue(KEY_SHORTCUT_FULL, getShortcut(Config::shortcutFullScreen));
    _shortcutSettings->setValue(KEY_SHORTCUT_ACTW, getShortcut(Config::shortcutActiveWnd));
    _shortcutSettings->setValue(KEY_SHORTCUT_AREA, getShortcut(Config::shortcutAreaSelect));
    _shortcutSettings->endGroup();
}

void ShortcutManager::setDefaultSettings()
{
    setShortcut(DEF_SHORTCUT_NEW,Config::shortcutNew, Config::localShortcut);
    setShortcut(DEF_SHORTCUT_SAVE,Config::shortcutSave, Config::localShortcut);
    setShortcut(DEF_SHORTCUT_COPY,Config::shortcutCopy, Config::localShortcut);
    setShortcut(DEF_SHORTCUT_OPT,Config::shortcutOptions, Config::localShortcut);
    setShortcut(DEF_SHORTCUT_HELP,Config::shortcutHelp, Config::localShortcut);
    setShortcut(DEF_SHORTCUT_CLOSE,Config::shortcutClose, Config::localShortcut);

    setShortcut(DEF_SHORTCUT_FULL,Config::shortcutFullScreen, Config::globalShortcut);
    setShortcut(DEF_SHORTCUT_ACTW,Config::shortcutActiveWnd, Config::globalShortcut);
    setShortcut(DEF_SHORTCUT_AREA,Config::shortcutAreaSelect, Config::globalShortcut);
}

void ShortcutManager::setShortcut(const QString &key, int action, int type)
{
    _listShortcuts[action].key = key;
    _listShortcuts[action].action = action;
    _listShortcuts[action].type = type;
}

QKeySequence ShortcutManager::getShortcut(int action)
{
    return QKeySequence(_listShortcuts[action].key);;
}

int ShortcutManager::getShortcutType(int action)
{
    return _listShortcuts[action].type;
}

QStringList ShortcutManager::getShortcutsList(int type)
{
    QStringList retList;
    for (int i = Config::shortcutFullScreen; i <= Config::shortcutClose; ++i)
    {
        if (_listShortcuts[i].type == type)
        {
            if (!_listShortcuts[i].key.isNull())
                retList << _listShortcuts[i].key;
            else
                retList << QLatin1String("");
        }
    }
    return retList;
}
