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

#ifndef MODULEMANAGER_H
#define MODULEMANAGER_H

#include "modules/abstractmodule.h"

#include <QByteArray>
#include <QHash>
#include <QMap>
#include <QMenu>

#include <QAction>

const QByteArray MOD_EXT_EDIT = "extedit";

typedef QMap<QByteArray, AbstractModule*> ModuleList_t;

class ModuleManager
{
public:
    ModuleManager();
    void initModules();
    AbstractModule* getModule(const QByteArray& name);
    AbstractModule* getModule(const quint8 numid);
    QList<QMenu*> generateModulesMenus(const QStringList &modules = QStringList());
    QList<QAction*> generateModulesActions(const QStringList &modules = QStringList());
    quint8 count();

private:
    ModuleList_t *_modules;
};

#endif // MODULEMANAGER_H
