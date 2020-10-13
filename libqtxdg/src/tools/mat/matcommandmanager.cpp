/*
 * libqtxdg - An Qt implementation of freedesktop.org xdg specs
 * Copyright (C) 2018  Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "matcommandmanager.h"

#include "xdgmacros.h"

#include "matcommandinterface.h"

#include <QDebug>


MatCommandManager::MatCommandManager()
{
}

MatCommandManager::~MatCommandManager()
{
    qDeleteAll(mCommands);
    mCommands.clear();
}

void MatCommandManager::add(MatCommandInterface *cmd)
{
    mCommands.append(cmd);
}

QList<MatCommandInterface *> MatCommandManager::commands() const
{
    return mCommands;
}

QString MatCommandManager::descriptionsHelpText() const
{
    QString text;
    int longestName = 0;
    const QLatin1String doubleSpace("  ");

    for (const auto *cmd : qAsConst(mCommands)) {
        longestName = qMax(longestName, cmd->name().size());
    }
    longestName += 2; // account for the inital dobule space
    for (const auto *cmd : qAsConst(mCommands)) {
        QString ptext = doubleSpace + cmd->name();
        ptext = ptext.leftJustified(longestName, QL1C(' '));
        ptext += doubleSpace + cmd->description() + QL1C('\n');
        text.append(ptext);
    }
    return text;
}
