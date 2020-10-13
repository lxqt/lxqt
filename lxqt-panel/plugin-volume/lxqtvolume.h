/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Johannes Zellner <webmaster@nebulon.de>
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

#ifndef LXQTVOLUME_H
#define LXQTVOLUME_H

#include "../panel/ilxqtpanelplugin.h"
#include <QToolButton>
#include <QSlider>
#include <QPointer>

class VolumeButton;
class AudioEngine;
class AudioDevice;
namespace LXQt {
class Notification;
}
namespace GlobalKeyShortcut
{
class Action;
}

class LXQtVolumeConfiguration;

class LXQtVolume : public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT
public:
    LXQtVolume(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~LXQtVolume();

    virtual QWidget *widget();
    virtual QString themeId() const { return QStringLiteral("Volume"); }
    virtual ILXQtPanelPlugin::Flags flags() const { return PreferRightAlignment | HaveConfigDialog ; }
    void realign();
    QDialog *configureDialog();

    void setAudioEngine(AudioEngine *engine);
protected slots:
    virtual void settingsChanged();
    void handleSinkListChanged();
    void handleShortcutVolumeUp();
    void handleShortcutVolumeDown();
    void handleShortcutVolumeMute();
    void shortcutRegistered();
    void showNotification(bool forceShow) const;

private:
    AudioEngine *m_engine;
    VolumeButton *m_volumeButton;
    int m_defaultSinkIndex;
    AudioDevice *m_defaultSink;
    GlobalKeyShortcut::Action *m_keyVolumeUp;
    GlobalKeyShortcut::Action *m_keyVolumeDown;
    GlobalKeyShortcut::Action *m_keyMuteToggle;
    LXQt::Notification *m_notification;
    QPointer<LXQtVolumeConfiguration> m_configDialog;
    bool m_allwaysShowNotifications;
    bool m_showKeyboardNotifications;
};


class LXQtVolumePluginLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const
    {
        return new LXQtVolume(startupInfo);
    }
};

#endif // LXQTVOLUME_H
