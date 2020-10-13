/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
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

#include "ossengine.h"
#include "audiodevice.h"
#include <QDebug>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/soundcard.h>
#elif defined(__linux__) || defined(__Linux__)
#include <linux/soundcard.h>
#else
#error "Not supported platform"
#endif

OssEngine::OssEngine(QObject *parent) :
    AudioEngine(parent),
    m_mixer(-1),
    m_device(nullptr),
    m_leftVolume(0),
    m_rightVolume(0)
{
    qDebug() << "OssEngine";
    initMixer();
}

OssEngine::~OssEngine()
{
    if(m_mixer >= 0)
        close(m_mixer);
}

void OssEngine::initMixer() {
    m_mixer = open ("/dev/mixer", O_RDWR, 0);
    if (m_mixer < 0) {
      qDebug() << "/dev/mixer cannot be opened";
      return;
    }
    qDebug() << "InitMixer:" << m_mixer;

    m_device = new AudioDevice(Sink, this);
    m_device->setName(QStringLiteral("Master"));
    m_device->setIndex(0);
    m_device->setDescription(QStringLiteral("Master Volume"));
    m_device->setMuteNoCommit(false);
    updateVolume();

    m_sinks.append(m_device);
    emit sinkListChanged();
}

void OssEngine::updateVolume() {
    if(m_mixer < 0 || !m_device)
        return;
    int volumes;
    if(ioctl(m_mixer, MIXER_READ(SOUND_MIXER_VOLUME), &volumes) < 0) {
        qDebug() << "updateVolume() failed" << errno;
    }
    m_leftVolume = volumes & 0xff; // left
    m_rightVolume = (volumes >> 8) & 0xff; // right
    qDebug() << "volume:" << m_leftVolume << m_rightVolume;

    m_device->setVolumeNoCommit(m_leftVolume);
}

void OssEngine::setVolume(int volume) {
    if(m_mixer < 0)
        return;
    int volumes = (volume << 8) + volume;
    if(ioctl(m_mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &volumes) < 0) {
        qDebug() << "setVolume() failed" << errno;
    }
    else {
        qDebug() << "setVolume()" << volume;
    }
}

void OssEngine::commitDeviceVolume(AudioDevice *device)
{
    if (!device)
        return;
    setVolume(device->volume());
}

void OssEngine::setMute(AudioDevice * /*device*/, bool state)
{
  if(state)
      setVolume(0);
  else
      setVolume(m_leftVolume);
}

void OssEngine::setIgnoreMaxVolume(bool /*ignore*/)
{
  // TODO
}
