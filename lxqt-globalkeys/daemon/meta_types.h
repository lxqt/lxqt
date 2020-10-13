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

#ifndef GLOBAL_ACTION__META_TYPES__INCLUDED
#define GLOBAL_ACTION__META_TYPES__INCLUDED


#include <QtGlobal>
#include <QDBusMetaType>


typedef enum MultipleActionsBehaviour
{
    MULTIPLE_ACTIONS_BEHAVIOUR_FIRST = 0, // queue
    MULTIPLE_ACTIONS_BEHAVIOUR_LAST,      // stack
    MULTIPLE_ACTIONS_BEHAVIOUR_NONE,      // qtcreator style
    MULTIPLE_ACTIONS_BEHAVIOUR_ALL,       // permissive behaviour
    MULTIPLE_ACTIONS_BEHAVIOUR__COUNT
} MultipleActionsBehaviour;

typedef struct CommonActionInfo
{
    QString shortcut;
    QString description;
    bool enabled;
} CommonActionInfo;

typedef struct GeneralActionInfo : CommonActionInfo
{
    QString type;
    QString info;
} GeneralActionInfo;

typedef struct ClientActionInfo : CommonActionInfo
{
    QDBusObjectPath path;
} ClientActionInfo;

typedef struct MethodActionInfo : CommonActionInfo
{
    QString service;
    QDBusObjectPath path;
    QString interface;
    QString method;
} MethodActionInfo;

typedef struct CommandActionInfo : CommonActionInfo
{
    QString command;
    QStringList arguments;
} CommandActionInfo;


typedef QMap<qulonglong, GeneralActionInfo> QMap_qulonglong_GeneralActionInfo;

Q_DECLARE_METATYPE(GeneralActionInfo)
Q_DECLARE_METATYPE(QMap_qulonglong_GeneralActionInfo)


QDBusArgument &operator << (QDBusArgument &argument, const GeneralActionInfo &generalActionInfo);
const QDBusArgument &operator >> (const QDBusArgument &argument, GeneralActionInfo &generalActionInfo);

#endif // GLOBAL_ACTION_MANAGER__META_TYPES__INCLUDED
