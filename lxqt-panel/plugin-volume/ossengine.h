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

#ifndef OSSENGINE_H
#define OSSENGINE_H

#include "audioengine.h"

#include <QObject>
#include <QList>
#include <QTimer>

class AudioDevice;

class OssEngine : public AudioEngine
{
    Q_OBJECT

public:
    OssEngine(QObject *parent = nullptr);
    ~OssEngine();

    virtual const QString backendName() const { return QLatin1String("Oss"); }
    virtual int volumeMax(AudioDevice */*device*/) const { return 100; }

    virtual void commitDeviceVolume(AudioDevice *device);
    virtual void setMute(AudioDevice *device, bool state);
    virtual void setIgnoreMaxVolume(bool ignore);

signals:
    void sinkInfoChanged(AudioDevice *device);
    void readyChanged(bool ready);

private:
    void initMixer();
    void updateVolume();
    void setVolume(int volume);

private:
    int m_mixer; // oss mixer fd
    AudioDevice* m_device;
    int m_leftVolume;
    int m_rightVolume;
};

#endif // OSSENGINE_H
