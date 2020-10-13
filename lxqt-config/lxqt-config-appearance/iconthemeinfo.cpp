/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Luis Pereira <luis.artur.pereira@gmail.com>
 *
 * The directoryMatchesSize() and thedirectorySizeDistance() functions were
 * taken from Qt5 qtbase/src/gui/image/qiconloader.cpp
 * Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "iconthemeinfo.h"

#define PREVIEW_ICON_SIZE 22

IconThemeInfo::IconThemeInfo(const QDir &dir):
    mValid(false),
    mHidden(false)
{
    mName = dir.dirName();
    if (dir.exists(QStringLiteral("index.theme")))
        load(dir.absoluteFilePath(QStringLiteral("index.theme")));
}


void IconThemeInfo::load(const QString &fileName)
{
    mFileName = fileName;
    mValid = false;
    QSettings file(mFileName, QSettings::IniFormat);
    if (file.status() != QSettings::NoError)
        return;

    if (file.value(QStringLiteral("Icon Theme/Directories")).toStringList().join(QLatin1Char(' ')).isEmpty())
        return;

    mHidden = file.value(QStringLiteral("Icon Theme/Hidden"), false).toBool();
    mText = file.value(QStringLiteral("Icon Theme/Name")).toString();
    mComment = file.value(QStringLiteral("Icon Theme/Comment")).toString();

    mValid = true;
}


QVector<QIcon> IconThemeInfo::icons(const QStringList &iconNames) const
{
    QVector<QIcon> icons;
    QString current_theme = QIcon::themeName();

    QIcon::setThemeName(mName);
    for (const auto & i : iconNames)
    {
        icons.push_back({QIcon::fromTheme(i).pixmap({PREVIEW_ICON_SIZE, PREVIEW_ICON_SIZE})});
    }

    QIcon::setThemeName(current_theme);

    return icons;
}
