/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
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

#ifndef LXQTVOLUMECONFIGURATION_H
#define LXQTVOLUMECONFIGURATION_H

#include "../panel/lxqtpanelpluginconfigdialog.h"
#include "../panel/pluginsettings.h"

#include <QList>

#define SETTINGS_MIXER_COMMAND          "mixerCommand"
#define SETTINGS_SHOW_ON_LEFTCLICK      "showOnLeftClick"
#define SETTINGS_MUTE_ON_MIDDLECLICK    "showOnMiddleClick"
#define SETTINGS_DEVICE                 "device"
#define SETTINGS_STEP                   "volumeAdjustStep"
#define SETTINGS_IGNORE_MAX_VOLUME      "ignoreMaxVolume"
#define SETTINGS_AUDIO_ENGINE           "audioEngine"
#define SETTINGS_ALLWAYS_SHOW_NOTIFICATIONS "allwaysShowNotifications"
#define SETTINGS_SHOW_KEYBOARD_NOTIFICATIONS "showKeyboardNotifications"

#define SETTINGS_DEFAULT_SHOW_ON_LEFTCLICK      true
#define SETTINGS_DEFAULT_MUTE_ON_MIDDLECLICK    true
#define SETTINGS_DEFAULT_DEVICE                 0
#define SETTINGS_DEFAULT_STEP                   3
#ifdef USE_PULSEAUDIO
    #define SETTINGS_DEFAULT_MIXER_COMMAND      "pavucontrol-qt"
    #define SETTINGS_DEFAULT_AUDIO_ENGINE       "PulseAudio"
#elif defined(USE_ALSA)
    #define SETTINGS_DEFAULT_MIXER_COMMAND      "qasmixer"
    #define SETTINGS_DEFAULT_AUDIO_ENGINE       "Alsa"
#else
    #define SETTINGS_DEFAULT_MIXER_COMMAND      ""
    #define SETTINGS_DEFAULT_AUDIO_ENGINE       "Oss"
#endif
#define SETTINGS_DEFAULT_IGNORE_MAX_VOLUME      false
#define SETTINGS_DEFAULT_IGNORE_MAX_VOLUME      false
#define SETTINGS_DEFAULT_ALLWAYS_SHOW_NOTIFICATIONS false
#define SETTINGS_DEFAULT_SHOW_KEYBOARD_NOTIFICATIONS true

class AudioDevice;

namespace Ui {
    class LXQtVolumeConfiguration;
}

class LXQtVolumeConfiguration : public LXQtPanelPluginConfigDialog
{
    Q_OBJECT

public:
    explicit LXQtVolumeConfiguration(PluginSettings *settings, bool ossAvailable, QWidget *parent = nullptr);
    ~LXQtVolumeConfiguration();

public slots:
    void setSinkList(const QList<AudioDevice*> sinks);
    void audioEngineChanged(bool checked);
    void sinkSelectionChanged(int index);
    void showOnClickedChanged(bool state);
    void muteOnMiddleClickChanged(bool state);
    void mixerLineEditChanged(const QString &command);
    void stepSpinBoxChanged(int step);
    void ignoreMaxVolumeCheckBoxChanged(bool state);
    void allwaysShowNotificationsCheckBoxChanged(bool state);
    void showKeyboardNotificationsCheckBoxChanged(bool state);

protected slots:
    virtual void loadSettings();

private:
    Ui::LXQtVolumeConfiguration *ui;
};

#endif // LXQTVOLUMECONFIGURATION_H
