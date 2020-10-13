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

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QList>
#include <QTimer>

class AudioDevice;

class AudioEngine : public QObject
{
    Q_OBJECT

public:
    AudioEngine(QObject *parent = nullptr);
    ~AudioEngine();

    const QList<AudioDevice *> &sinks() const { return m_sinks; }
    virtual int volumeMax(AudioDevice *device) const = 0;
    virtual int volumeBounded(int volume, AudioDevice *device) const;
    virtual const QString backendName() const = 0;

public slots:
    virtual void commitDeviceVolume(AudioDevice *device) = 0;
    virtual void setMute(AudioDevice *device, bool state) = 0;
    void mute(AudioDevice *device);
    void unmute(AudioDevice *device);
    virtual void setIgnoreMaxVolume(bool ignore);

signals:
    void sinkListChanged();

protected:
    QList<AudioDevice*> m_sinks;
};

#endif // AUDIOENGINE_H
