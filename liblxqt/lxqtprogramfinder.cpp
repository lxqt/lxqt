/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright (C) 2013  Alec Moskvin <alecm@gmx.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "lxqtprogramfinder.h"
#include <wordexp.h>
#include <QDir>
#include <QFileInfo>

using namespace LXQt;

LXQT_API bool ProgramFinder::programExists(const QString& command)
{
    const QString program = programName(command);
    if (program[0] == QL1C('/'))
    {
        QFileInfo fi(program);
        return fi.isExecutable() && fi.isFile();
    }

    const QString path = QFile::decodeName(qgetenv("PATH"));
    const QStringList dirs = path.split(QL1C(':'), QString::SkipEmptyParts);
    for (const QString& dirName : dirs)
    {
        const QFileInfo fi(QDir(dirName), program);
        if (fi.isExecutable() && fi.isFile())
            return true;
    }
    return false;
}

LXQT_API QStringList ProgramFinder::findPrograms(const QStringList& commands)
{
    QStringList availPrograms;
    for (const QString& program : commands)
        if (programExists(program))
            availPrograms.append(program);
    return availPrograms;
}

LXQT_API QString ProgramFinder::programName(const QString& command)
{
    wordexp_t we;
    if (wordexp(command.toLocal8Bit().constData(), &we, WRDE_NOCMD) == 0)
        if (we.we_wordc > 0)
            return QString::fromLocal8Bit(we.we_wordv[0]);
    return QString();
}
