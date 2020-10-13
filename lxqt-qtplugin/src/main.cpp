/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2014 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

// NOTE: To load the plugin, we can set environment variable QT_QPA_PLATFORMTHEME=lxqt

#include <qpa/qplatformthemeplugin.h>
#include "lxqtplatformtheme.h"

QT_BEGIN_NAMESPACE

class LXQtPlatformThemePlugin: public QPlatformThemePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QPA.QPlatformThemeFactoryInterface.5.1" FILE "lxqtplatformtheme.json")
public:
    QPlatformTheme *create(const QString &key, const QStringList &params) override;
};

QPlatformTheme *LXQtPlatformThemePlugin::create(const QString &key, const QStringList &/*params*/) {
    if (!key.compare(QLatin1String("lxqt"), Qt::CaseInsensitive))
        return new LXQtPlatformTheme();
    return nullptr;
}

QT_END_NAMESPACE

#include "main.moc"
