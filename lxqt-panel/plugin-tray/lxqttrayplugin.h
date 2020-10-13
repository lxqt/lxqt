/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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


#ifndef LXQTTRAYPLUGIN_H
#define LXQTTRAYPLUGIN_H

#include "../panel/ilxqtpanelplugin.h"
#include <QDebug>
#include <QObject>
#include <QX11Info>

class LXQtTray;
class LXQtTrayPlugin : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    explicit LXQtTrayPlugin(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~LXQtTrayPlugin();

    virtual QWidget *widget();
    virtual QString themeId() const { return QStringLiteral("Tray"); }
    virtual Flags flags() const { return HaveConfigDialog | PreferRightAlignment | SingleInstance | NeedsHandle; }

    QDialog *configureDialog();

    void realign();
    void settingsChanged();

    bool isSeparate() const { return true; }

private:
    LXQtTray *mWidget;

};

class LXQtTrayPluginLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    // Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const
    {
        // Currently only X11 supported
        if (!QX11Info::connection()) {
            qWarning() << "Currently tray plugin supports X11 only. Skipping.";
            return nullptr;
        }
        return new LXQtTrayPlugin(startupInfo);
    }
};

#endif // LXQTTRAYPLUGIN_H
