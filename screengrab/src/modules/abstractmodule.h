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

#ifndef ABSTRACTMODULE_H
#define ABSTRACTMODULE_H

#include <QList>
#include <QWidget>
#include <QAction>
#include <QMenu>

class AbstractModule
{
public:
    AbstractModule() {};
    virtual ~AbstractModule() {};

    // interface
    virtual void init() = 0;
    virtual QString moduleName() = 0;
    virtual QWidget* initConfigWidget() = 0;
    virtual void defaultSettings() = 0;
    virtual QMenu* initModuleMenu() = 0;
    virtual QAction* initModuleAction() = 0;
};


#endif // ABSTRACTMODULE_H
