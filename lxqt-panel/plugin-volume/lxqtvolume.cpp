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

#include "lxqtvolume.h"

#include "volumebutton.h"
#include "volumepopup.h"
#include "lxqtvolumeconfiguration.h"
#include "audiodevice.h"
#ifdef USE_PULSEAUDIO
#include "pulseaudioengine.h"
#endif
#ifdef USE_ALSA
#include "alsaengine.h"
#endif
#include "ossengine.h"

#include <QMessageBox>
#include <XdgIcon>
#include <lxqt-globalkeys.h>
#include <LXQt/Notification>

#define DEFAULT_UP_SHORTCUT "XF86AudioRaiseVolume"
#define DEFAULT_DOWN_SHORTCUT "XF86AudioLowerVolume"
#define DEFAULT_MUTE_SHORTCUT "XF86AudioMute"

LXQtVolume::LXQtVolume(const ILXQtPanelPluginStartupInfo &startupInfo):
        QObject(),
        ILXQtPanelPlugin(startupInfo),
        m_engine(0),
        m_defaultSinkIndex(0),
        m_defaultSink(0),
        m_allwaysShowNotifications(SETTINGS_DEFAULT_ALLWAYS_SHOW_NOTIFICATIONS),
        m_showKeyboardNotifications(SETTINGS_DEFAULT_SHOW_KEYBOARD_NOTIFICATIONS)
{
    m_volumeButton = new VolumeButton(this);

    m_notification = new LXQt::Notification(QLatin1String(""), this);

    m_keyVolumeUp = GlobalKeyShortcut::Client::instance()->addAction(QString(), QStringLiteral("/panel/%1/up").arg(settings()->group()), tr("Increase sound volume"), this);
    if (m_keyVolumeUp)
    {
        connect(m_keyVolumeUp, &GlobalKeyShortcut::Action::registrationFinished, this, &LXQtVolume::shortcutRegistered);
        connect(m_keyVolumeUp, SIGNAL(activated()), this, SLOT(handleShortcutVolumeUp()));
    }
    m_keyVolumeDown = GlobalKeyShortcut::Client::instance()->addAction(QString(), QStringLiteral("/panel/%1/down").arg(settings()->group()), tr("Decrease sound volume"), this);
    if (m_keyVolumeDown)
    {
        connect(m_keyVolumeDown, &GlobalKeyShortcut::Action::registrationFinished, this, &LXQtVolume::shortcutRegistered);
        connect(m_keyVolumeDown, SIGNAL(activated()), this, SLOT(handleShortcutVolumeDown()));
    }
    m_keyMuteToggle = GlobalKeyShortcut::Client::instance()->addAction(QString(), QStringLiteral("/panel/%1/mute").arg(settings()->group()), tr("Mute/unmute sound volume"), this);
    if (m_keyMuteToggle)
    {
        connect(m_keyMuteToggle, &GlobalKeyShortcut::Action::registrationFinished, this, &LXQtVolume::shortcutRegistered);
        connect(m_keyMuteToggle, SIGNAL(activated()), this, SLOT(handleShortcutVolumeMute()));
    }

    settingsChanged();
}

LXQtVolume::~LXQtVolume()
{
    delete m_volumeButton;
}

void LXQtVolume::shortcutRegistered()
{
    GlobalKeyShortcut::Action * const shortcut = qobject_cast<GlobalKeyShortcut::Action*>(sender());

    QString shortcutNotRegistered;

    if (shortcut == m_keyVolumeUp)
    {
        disconnect(m_keyVolumeUp, &GlobalKeyShortcut::Action::registrationFinished, this, &LXQtVolume::shortcutRegistered);

        if (m_keyVolumeUp->shortcut().isEmpty())
        {
            m_keyVolumeUp->changeShortcut(QStringLiteral(DEFAULT_UP_SHORTCUT));
            if (m_keyVolumeUp->shortcut().isEmpty())
            {
                shortcutNotRegistered = QStringLiteral(" '") + QStringLiteral(DEFAULT_UP_SHORTCUT) + QStringLiteral("'");
            }
        }
    } else if (shortcut == m_keyVolumeDown)
    {
        disconnect(m_keyVolumeDown, &GlobalKeyShortcut::Action::registrationFinished, this, &LXQtVolume::shortcutRegistered);

        if (m_keyVolumeDown->shortcut().isEmpty())
        {
            m_keyVolumeDown->changeShortcut(QStringLiteral(DEFAULT_DOWN_SHORTCUT));
            if (m_keyVolumeDown->shortcut().isEmpty())
            {
                shortcutNotRegistered += QStringLiteral(" '") + QStringLiteral(DEFAULT_DOWN_SHORTCUT) + QStringLiteral("'");
            }
        }
    } else if (shortcut == m_keyMuteToggle)
    {
        disconnect(m_keyMuteToggle, &GlobalKeyShortcut::Action::registrationFinished, this, &LXQtVolume::shortcutRegistered);

        if (m_keyMuteToggle->shortcut().isEmpty())
        {
            m_keyMuteToggle->changeShortcut(QStringLiteral(DEFAULT_MUTE_SHORTCUT));
            if (m_keyMuteToggle->shortcut().isEmpty())
            {
                shortcutNotRegistered += QStringLiteral(" '") + QStringLiteral(DEFAULT_MUTE_SHORTCUT) + QStringLiteral("'");
            }
        }
    }

    if(!shortcutNotRegistered.isEmpty())
    {
        m_notification->setSummary(tr("Volume Control: The following shortcuts can not be registered: %1").arg(shortcutNotRegistered));
        m_notification->update();
    }

    m_notification->setTimeout(1000);
    m_notification->setUrgencyHint(LXQt::Notification::UrgencyLow);
}

void LXQtVolume::setAudioEngine(AudioEngine *engine)
{
    if (m_engine) {
        if (m_engine->backendName() == engine->backendName())
            return;

        if (m_defaultSink)
        {
            disconnect(m_defaultSink, nullptr, this, nullptr);
            disconnect(m_defaultSink, nullptr, this, nullptr);
            m_defaultSink = nullptr;
        }
        m_volumeButton->volumePopup()->setDevice(m_defaultSink);

        disconnect(m_engine, 0, 0, 0);
        delete m_engine;
        m_engine = 0;
    }

    m_engine = engine;
    connect(m_engine, &AudioEngine::sinkListChanged, this, &LXQtVolume::handleSinkListChanged);

    handleSinkListChanged();
}


void LXQtVolume::settingsChanged()
{
    m_defaultSinkIndex = settings()->value(QStringLiteral(SETTINGS_DEVICE), SETTINGS_DEFAULT_DEVICE).toInt();
    QString engineName = settings()->value(QStringLiteral(SETTINGS_AUDIO_ENGINE), QStringLiteral(SETTINGS_DEFAULT_AUDIO_ENGINE)).toString();
    const bool new_engine = !m_engine || m_engine->backendName() != engineName;
    if (new_engine) {
#if defined(USE_PULSEAUDIO) && defined(USE_ALSA)
        if (engineName == QLatin1String("PulseAudio"))
            setAudioEngine(new PulseAudioEngine(this));
        else if (engineName == QLatin1String("Alsa"))
            setAudioEngine(new AlsaEngine(this));
        else // fallback to OSS
            setAudioEngine(new OssEngine(this));
#elif defined(USE_PULSEAUDIO)
        if (engineName == QLatin1String("PulseAudio"))
            setAudioEngine(new PulseAudioEngine(this));
        else // fallback to OSS
            setAudioEngine(new OssEngine(this));
#elif defined(USE_ALSA)
        if (engineName == QLatin1String("Alsa"))
            setAudioEngine(new AlsaEngine(this));
        else // fallback to OSS
            setAudioEngine(new OssEngine(this));
#else
        // No other backends are available, fallback to OSS
        setAudioEngine(new OssEngine(this));
#endif
    }

    m_volumeButton->setShowOnClicked(settings()->value(QStringLiteral(SETTINGS_SHOW_ON_LEFTCLICK), SETTINGS_DEFAULT_SHOW_ON_LEFTCLICK).toBool());
    m_volumeButton->setMuteOnMiddleClick(settings()->value(QStringLiteral(SETTINGS_MUTE_ON_MIDDLECLICK), SETTINGS_DEFAULT_MUTE_ON_MIDDLECLICK).toBool());
    m_volumeButton->setMixerCommand(settings()->value(QStringLiteral(SETTINGS_MIXER_COMMAND), QStringLiteral(SETTINGS_DEFAULT_MIXER_COMMAND)).toString());
    m_volumeButton->volumePopup()->setSliderStep(settings()->value(QStringLiteral(SETTINGS_STEP), SETTINGS_DEFAULT_STEP).toInt());
    m_allwaysShowNotifications = settings()->value(QStringLiteral(SETTINGS_ALLWAYS_SHOW_NOTIFICATIONS), SETTINGS_DEFAULT_ALLWAYS_SHOW_NOTIFICATIONS).toBool();
    m_showKeyboardNotifications = settings()->value(QStringLiteral(SETTINGS_SHOW_KEYBOARD_NOTIFICATIONS), SETTINGS_DEFAULT_SHOW_KEYBOARD_NOTIFICATIONS).toBool()
                                  // in case the config file was edited manually (see LXQtVolumeConfiguration)
                                  || m_allwaysShowNotifications;

    if (!new_engine)
        handleSinkListChanged();
}

void LXQtVolume::handleSinkListChanged()
{
    if (m_engine)
    {
        if (m_engine->sinks().count() > 0)
        {
            m_defaultSink = m_engine->sinks().at(qBound(0, m_defaultSinkIndex, m_engine->sinks().count()-1));
            m_volumeButton->volumePopup()->setDevice(m_defaultSink);
            connect(m_defaultSink, &AudioDevice::volumeChanged, this, [this] { LXQtVolume::showNotification(false); });
            connect(m_defaultSink, &AudioDevice::muteChanged, this, [this] { LXQtVolume::showNotification(false); });

            m_engine->setIgnoreMaxVolume(settings()->value(QStringLiteral(SETTINGS_IGNORE_MAX_VOLUME), SETTINGS_DEFAULT_IGNORE_MAX_VOLUME).toBool());
        }

        if (m_configDialog)
            m_configDialog->setSinkList(m_engine->sinks());
    }
}

void LXQtVolume::handleShortcutVolumeUp()
{
    if (m_defaultSink)
    {
        m_defaultSink->setVolume(m_defaultSink->volume() + settings()->value(QStringLiteral(SETTINGS_STEP), SETTINGS_DEFAULT_STEP).toInt());
        showNotification(true);
    }
}

void LXQtVolume::handleShortcutVolumeDown()
{
    if (m_defaultSink)
    {
        m_defaultSink->setVolume(m_defaultSink->volume() - settings()->value(QStringLiteral(SETTINGS_STEP), SETTINGS_DEFAULT_STEP).toInt());
        showNotification(true);
    }
}

void LXQtVolume::handleShortcutVolumeMute()
{
    if (m_defaultSink)
    {
        m_defaultSink->toggleMute();
        showNotification(true);
    }
}

QWidget *LXQtVolume::widget()
{
    return m_volumeButton;
}

void LXQtVolume::realign()
{
}

QDialog *LXQtVolume::configureDialog()
{
    if (!m_configDialog)
    {
        const bool oss_available = (m_engine && m_engine->backendName() == QLatin1String("Oss"))
            ? m_engine->sinks().size() > 0
            : OssEngine().sinks().size() > 0;
        m_configDialog = new LXQtVolumeConfiguration(settings(), oss_available);
        m_configDialog->setAttribute(Qt::WA_DeleteOnClose, true);

        if (m_engine)
           m_configDialog->setSinkList(m_engine->sinks());
    }
    return m_configDialog;
}

void LXQtVolume::showNotification(bool forceShow) const
{
    if ((forceShow && m_showKeyboardNotifications)  // force only if volume change should be notified with keyboard
        || m_allwaysShowNotifications)
    {
        if (Q_LIKELY(m_defaultSink))
        {
            m_notification->setSummary(tr("Volume: %1%%2").arg(QString::number(m_defaultSink->volume())).arg(m_defaultSink->mute() ? tr("(muted)") : QLatin1String("")));
            m_notification->update();
        }
    }
}

#undef DEFAULT_UP_SHORTCUT
#undef DEFAULT_DOWN_SHORTCUT
#undef DEFAULT_MUTE_SHORTCUT
