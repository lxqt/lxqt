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

#ifndef PULSEAUDIOENGINE_H
#define PULSEAUDIOENGINE_H

#include "audioengine.h"

#include <QObject>
#include <QList>
#include <QTimer>
#include <QMap>

#include <pulse/pulseaudio.h>

// PA_VOLUME_UI_MAX is only supported since pulseaudio 0.9.23
#ifndef PA_VOLUME_UI_MAX
#define PA_VOLUME_UI_MAX (pa_sw_volume_from_dB(+11.0))
#endif

class AudioDevice;

class PulseAudioEngine : public AudioEngine
{
    Q_OBJECT

public:
    PulseAudioEngine(QObject *parent = nullptr);
    ~PulseAudioEngine();

    virtual const QString backendName() const { return QLatin1String("PulseAudio"); }

    int volumeMax(AudioDevice */*device*/) const { return m_maximumVolume; }

    void requestSinkInfoUpdate(uint32_t idx);
    void removeSink(uint32_t idx);
    void addOrUpdateSink(const pa_sink_info *info);

    pa_context_state_t contextState() const { return m_contextState; }
    bool ready() const { return m_ready; }
    pa_threaded_mainloop *mainloop() const { return m_mainLoop; }

public slots:
    void commitDeviceVolume(AudioDevice *device);
    void retrieveSinkInfo(uint32_t idx);
    void setMute(AudioDevice *device, bool state);
    void setContextState(pa_context_state_t state);
    void setIgnoreMaxVolume(bool ignore);

signals:
    void sinkInfoChanged(uint32_t idx);
    void contextStateChanged(pa_context_state_t state);
    void readyChanged(bool ready);

private slots:
    void handleContextStateChanged();
    void connectContext();

private:
    void retrieveSinks();
    void setupSubscription();

    pa_mainloop_api *m_mainLoopApi;
    pa_threaded_mainloop *m_mainLoop;
    pa_context *m_context;

    pa_context_state_t m_contextState;
    bool m_ready;
    QTimer m_reconnectionTimer;
    int m_maximumVolume;

    QMap<AudioDevice *, pa_cvolume> m_cVolumeMap;
};

#endif // PULSEAUDIOENGINE_H
