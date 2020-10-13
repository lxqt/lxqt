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

#include "alsadevice.h"

AlsaDevice::AlsaDevice(AudioDeviceType t, AudioEngine *engine, QObject *parent) :
    AudioDevice(t, engine, parent),
    m_mixer(0),
    m_elem(0),
    m_volumeMin(0),
    m_volumeMax(100)
{
}

void AlsaDevice::setMixer(snd_mixer_t *mixer)
{
    if (m_mixer == mixer)
        return;

    m_mixer = mixer;
    emit mixerChanged();
}

void AlsaDevice::setElement(snd_mixer_elem_t *elem)
{
    if (m_elem == elem)
        return;

    m_elem = elem;
    emit elementChanged();
}

void AlsaDevice::setCardName(const QString &cardName)
{
    if (m_cardName == cardName)
        return;

    m_cardName = cardName;
    emit cardNameChanged();
}

void AlsaDevice::setVolumeMinMax(long volumeMin, long volumeMax)
{
    m_volumeMin = volumeMin;
    m_volumeMax = volumeMax;
}
