/*
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright (C) 2018  Luís Pereira <luis.artur.pereira@gmail.com>
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
 */

#include "lxqtconfigdialogcmdlineoptions.h"

#include <QCommandLineParser>

namespace LXQt {

class Q_DECL_HIDDEN ConfigDialogCmdLineOptionsPrivate {
public:
    QString mPage;
};

ConfigDialogCmdLineOptions::ConfigDialogCmdLineOptions()
    : d(new ConfigDialogCmdLineOptionsPrivate)
{
}

ConfigDialogCmdLineOptions::~ConfigDialogCmdLineOptions()
{
}

bool ConfigDialogCmdLineOptions::setCommandLine(QCommandLineParser *parser)
{
    Q_ASSERT(parser);
    if (!parser)
        return false;

    return parser->addOption(QCommandLineOption{QStringList{QL1S("s"), QL1S("show-page")}, QCoreApplication::tr("Choose the page to be shown."), QL1S("name")});
}

void ConfigDialogCmdLineOptions::process(QCommandLineParser &parser)
{
    if (parser.isSet(QL1S("show-page"))) {
        d->mPage = parser.value(QL1S("show-page"));
    }
}

QString ConfigDialogCmdLineOptions::page() const
{
    return d->mPage;
}

} // LXQt namespace
