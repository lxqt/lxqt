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

#include "command_action.h"

#include <QProcess>

#include <cerrno>
#include <cstring>

#include "log_target.h"
#include "string_utils.h"


CommandAction::CommandAction(LogTarget *logTarget, const QString &command, const QStringList &args, const QString &description)
    : BaseAction(logTarget, description)
    , mCommand(command)
    , mArgs(args)
{
}

bool CommandAction::call()
{
    if (!isEnabled())
    {
        return false;
    }

    bool result = QProcess::startDetached(mCommand, mArgs);
    if (!result)
    {
        mLogTarget->log(LOG_WARNING, "Failed to launch command \"%s\"%s", qPrintable(mCommand), qPrintable(joinToString(mArgs, QStringLiteral(" \""), QStringLiteral("\" \""), QStringLiteral("\""))));
    }

    return result;
}
