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

#ifndef ALSADEVICE_H
#define ALSADEVICE_H

#include "audiodevice.h"

#include <alsa/asoundlib.h>

#include <QObject>
#include <QString>

class AlsaDevice : public AudioDevice
{
    Q_OBJECT

public:
    AlsaDevice(AudioDeviceType t, AudioEngine *engine, QObject *parent = nullptr);

    snd_mixer_t *mixer() const { return m_mixer; }
    snd_mixer_elem_t *element() const { return m_elem; }
    const QString &cardName() const { return m_cardName; }
    inline long volumeMin() const { return m_volumeMin; }
    inline long volumeMax() const { return m_volumeMax; }

    void setMixer(snd_mixer_t *mixer);
    void setElement(snd_mixer_elem_t *elem);
    void setCardName(const QString &cardName);
    void setVolumeMinMax(long volumeMin, long volumeMax);

signals:
    void mixerChanged();
    void elementChanged();
    void cardNameChanged();

private:
    snd_mixer_t *m_mixer;
    snd_mixer_elem_t *m_elem;
    QString m_cardName;
    long m_volumeMin;
    long m_volumeMax;
};

#endif // ALSADEVICE_H
