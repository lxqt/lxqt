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

#include "modulemanager.h"

#ifdef SG_EXT_EDIT
#include "src/modules/extedit/moduleextedit.h"
#endif

#include <QDebug>

ModuleManager::ModuleManager()
{
    _modules = new ModuleList_t();
}

void ModuleManager::initModules()
{
#ifdef SG_EXT_EDIT
    ModuleExtEdit *extedit = new ModuleExtEdit();
    _modules->insert(MOD_EXT_EDIT.data(), extedit);
#endif
}

AbstractModule* ModuleManager::getModule(const QByteArray& name)
{
    if (_modules->contains(name))
        return _modules->value(name);
    return nullptr;
}

AbstractModule* ModuleManager::getModule(const quint8 numid)
{
    if (numid < _modules->count())
    {
        QByteArray key = _modules->keys().at(numid);
        return _modules->value(key);
    }

    return nullptr;
}

QList<QMenu*> ModuleManager::generateModulesMenus(const QStringList &modules)
{
    QList<QMenu*> list;
    if (modules.isEmpty() == true)
    {
        for (int i =0; i < _modules->keys().count(); ++i)
        {
            QMenu *menu = _modules->value(_modules->keys().at(i))->initModuleMenu();
            list.append(menu);
        }
    }
    else
    {
        for (int i = 0; i < modules.count(); ++i)
        {
            QByteArray currentKey = modules.at(i).toLatin1();
            QMenu *menu = _modules->value(currentKey)->initModuleMenu();
            list.append(menu);
        }
    }

    return list;
}

QList<QAction*> ModuleManager::generateModulesActions(const QStringList &modules)
{
    QList<QAction*> list;

    if (modules.isEmpty())
    {
        for (int i =0; i < _modules->keys().count(); ++i)
        {
            QAction *action = _modules->value(_modules->keys().at(i))->initModuleAction();
            list.append(action);
        }
    }
    else
    {
        for (int i = 0; i < modules.count(); ++i)
        {
            QByteArray currentKey = modules.at(i).toLatin1();
            QAction *action = _modules->value(currentKey)->initModuleAction();
            list.append(action);
        }
    }

    return list;
}

quint8 ModuleManager::count()
{
    return _modules->count();
}
