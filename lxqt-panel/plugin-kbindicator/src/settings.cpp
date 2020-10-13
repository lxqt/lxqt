/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Dmitriy Zhukov <zjesclean@gmail.com>
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

#include <QSettings>
#include "settings.h"

Settings::Settings()
{}

Settings &Settings::instance()
{
    static Settings _instance;
    return _instance;
}

void Settings::init(PluginSettings *settings)
{
    m_settings = settings;
}

bool Settings::showCapLock() const
{ return m_settings->value(QStringLiteral("show_caps_lock"), true).toBool(); }

bool Settings::showNumLock() const
{ return m_settings->value(QStringLiteral("show_num_lock"), true).toBool(); }

bool Settings::showScrollLock() const
{ return m_settings->value(QStringLiteral("show_scroll_lock"), true).toBool(); }

bool Settings::showLayout() const
{ return m_settings->value(QStringLiteral("show_layout"), true).toBool(); }

QString Settings::layoutFlagPattern() const
{ return m_settings->value(QStringLiteral("layout_flag_pattern")).toString(); }

void Settings::setShowCapLock(bool show)
{ m_settings->setValue(QStringLiteral("show_caps_lock"), show); }

void Settings::setShowNumLock(bool show)
{ m_settings->setValue(QStringLiteral("show_num_lock"), show); }

void Settings::setShowScrollLock(bool show)
{ m_settings->setValue(QStringLiteral("show_scroll_lock"), show); }

void Settings::setShowLayout(bool show)
{ m_settings->setValue(QStringLiteral("show_layout"), show); }

void Settings::setLayoutFlagPattern(const QString & layoutFlagPattern)
{ m_settings->setValue(QStringLiteral("layout_flag_pattern"), layoutFlagPattern); }

KeeperType Settings::keeperType() const
{
    QString type = m_settings->value(QStringLiteral("keeper_type"), QStringLiteral("global")).toString();
    if(type == QLatin1String("global"))
        return KeeperType::Global;
    if(type == QLatin1String("window"))
        return KeeperType::Window;
    if(type == QLatin1String("application"))
        return KeeperType::Application;
    return KeeperType::Application;
}

void Settings::setKeeperType(KeeperType type) const
{
    switch (type) {
    case KeeperType::Global:
        m_settings->setValue(QStringLiteral("keeper_type"), QStringLiteral("global"));
        break;
    case KeeperType::Window:
        m_settings->setValue(QStringLiteral("keeper_type"), QStringLiteral("window"));
        break;
    case KeeperType::Application:
        m_settings->setValue(QStringLiteral("keeper_type"), QStringLiteral("application"));
        break;
    }
}

void Settings::restore()
{ m_settings->loadFromCache(); }
