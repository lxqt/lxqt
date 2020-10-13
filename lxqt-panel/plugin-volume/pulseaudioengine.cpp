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

#include "pulseaudioengine.h"

#include "audiodevice.h"

#include <QMetaType>
#include <QtDebug>

//#define PULSEAUDIO_ENGINE_DEBUG

static void sinkInfoCallback(pa_context *context, const pa_sink_info *info, int isLast, void *userdata)
{
    PulseAudioEngine *pulseEngine = static_cast<PulseAudioEngine*>(userdata);
    QMap<pa_sink_state, QString> stateMap;
    stateMap[PA_SINK_INVALID_STATE] = QLatin1String("n/a");
    stateMap[PA_SINK_RUNNING] = QLatin1String("RUNNING");
    stateMap[PA_SINK_IDLE] = QLatin1String("IDLE");
    stateMap[PA_SINK_SUSPENDED] = QLatin1String("SUSPENDED");

    if (isLast < 0) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        qWarning() << QStringLiteral("Failed to get sink information: %1").arg(QString::fromUtf8(pa_strerror(pa_context_errno(context))));
        return;
    }

    if (isLast) {
        pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
        return;
    }

    pulseEngine->addOrUpdateSink(info);
}

static void contextEventCallback(pa_context * /*context*/, const char *
#ifdef PULSEAUDIO_ENGINE_DEBUG
        name
#endif
        , pa_proplist * /*p*/, void * /*userdata*/)
{
#ifdef PULSEAUDIO_ENGINE_DEBUG
    qWarning("event received %s", name);
#endif
}

static void contextStateCallback(pa_context *context, void *userdata)
{
    PulseAudioEngine *pulseEngine = reinterpret_cast<PulseAudioEngine*>(userdata);

    // update internal state
    pa_context_state_t state = pa_context_get_state(context);
    pulseEngine->setContextState(state);

#ifdef PULSEAUDIO_ENGINE_DEBUG
    switch (state) {
        case PA_CONTEXT_UNCONNECTED:
            qWarning("context unconnected");
            break;
        case PA_CONTEXT_CONNECTING:
            qWarning("context connecting");
            break;
        case PA_CONTEXT_AUTHORIZING:
            qWarning("context authorizing");
            break;
        case PA_CONTEXT_SETTING_NAME:
            qWarning("context setting name");
            break;
        case PA_CONTEXT_READY:
            qWarning("context ready");
            break;
        case PA_CONTEXT_FAILED:
            qWarning("context failed");
            break;
        case PA_CONTEXT_TERMINATED:
            qWarning("context terminated");
            break;
        default:
            qWarning("we should never hit this state");
    }
#endif

    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void contextSuccessCallback(pa_context *context, int success, void *userdata)
{
    Q_UNUSED(context);
    Q_UNUSED(success);
    Q_UNUSED(userdata);

    PulseAudioEngine *pulseEngine = reinterpret_cast<PulseAudioEngine*>(userdata);
    pa_threaded_mainloop_signal(pulseEngine->mainloop(), 0);
}

static void contextSubscriptionCallback(pa_context * /*context*/, pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
    PulseAudioEngine *pulseEngine = reinterpret_cast<PulseAudioEngine*>(userdata);
    if (PA_SUBSCRIPTION_EVENT_REMOVE == t)
        pulseEngine->removeSink(idx);
    else
        pulseEngine->requestSinkInfoUpdate(idx);
}


PulseAudioEngine::PulseAudioEngine(QObject *parent) :
    AudioEngine(parent),
    m_context(0),
    m_contextState(PA_CONTEXT_UNCONNECTED),
    m_ready(false),
    m_maximumVolume(PA_VOLUME_UI_MAX)
{
    qRegisterMetaType<pa_context_state_t>("pa_context_state_t");

    m_reconnectionTimer.setSingleShot(true);
    m_reconnectionTimer.setInterval(100);
    connect(&m_reconnectionTimer, SIGNAL(timeout()), this, SLOT(connectContext()));

    m_mainLoop = pa_threaded_mainloop_new();
    if (m_mainLoop == 0) {
        qWarning("Unable to create pulseaudio mainloop");
        return;
    }

    if (pa_threaded_mainloop_start(m_mainLoop) != 0) {
        qWarning("Unable to start pulseaudio mainloop");
        pa_threaded_mainloop_free(m_mainLoop);
        m_mainLoop = 0;
        return;
    }

    m_mainLoopApi = pa_threaded_mainloop_get_api(m_mainLoop);

    connect(this, SIGNAL(contextStateChanged(pa_context_state_t)), this, SLOT(handleContextStateChanged()));

    connectContext();
}

PulseAudioEngine::~PulseAudioEngine()
{
    if (m_context) {
        pa_context_unref(m_context);
        m_context = 0;
    }

    if (m_mainLoop) {
        pa_threaded_mainloop_free(m_mainLoop);
        m_mainLoop = 0;
    }
}

void PulseAudioEngine::removeSink(uint32_t idx)
{
    auto dev_i = std::find_if(m_sinks.begin(), m_sinks.end(), [idx] (AudioDevice * dev) { return dev->index() == idx; });
    if (m_sinks.end() == dev_i)
        return;

    QScopedPointer<AudioDevice> dev{*dev_i};
    m_cVolumeMap.remove(dev.data());
    m_sinks.erase(dev_i);
    emit sinkListChanged();
}

void PulseAudioEngine::addOrUpdateSink(const pa_sink_info *info)
{
    AudioDevice *dev = 0;
    bool newSink = false;
    QString name = QString::fromUtf8(info->name);

    for (AudioDevice *device : qAsConst(m_sinks)) {
        if (device->name() == name) {
            dev = device;
            break;
        }
    }

    if (!dev) {
        dev = new AudioDevice(Sink, this);
        newSink = true;
    }

    dev->setName(name);
    dev->setIndex(info->index);
    dev->setDescription(QString::fromUtf8(info->description));
    dev->setMuteNoCommit(info->mute);

    // TODO: save separately? alsa does not have it
    m_cVolumeMap.insert(dev, info->volume);

    pa_volume_t v = pa_cvolume_avg(&(info->volume));
    // convert real volume to percentage
    dev->setVolumeNoCommit(qRound((static_cast<double>(v) * 100.0) / m_maximumVolume));

    if (newSink) {
        //keep the sinks sorted by index()
        m_sinks.insert(
                std::lower_bound(m_sinks.begin(), m_sinks.end(), dev,  [] (AudioDevice const * const a, AudioDevice const * const b) {
                    return a->name() < b->name();
                    })
                , dev
                );
        emit sinkListChanged();
    }
}

void PulseAudioEngine::requestSinkInfoUpdate(uint32_t idx)
{
    emit sinkInfoChanged(idx);
}

void PulseAudioEngine::commitDeviceVolume(AudioDevice *device)
{
    if (!device || !m_ready)
        return;

    // convert from percentage to real volume value
    pa_volume_t v = ((double)device->volume() / 100.0) * m_maximumVolume;
    pa_cvolume tmpVolume = m_cVolumeMap.value(device);
    pa_cvolume *volume = pa_cvolume_set(&tmpVolume, tmpVolume.channels, v);
    // qDebug() << "PulseAudioEngine::commitDeviceVolume" << v;
    pa_threaded_mainloop_lock(m_mainLoop);

    pa_operation *operation;
    if (device->type() == Sink)
        operation = pa_context_set_sink_volume_by_index(m_context, device->index(), volume, contextSuccessCallback, this);
    else
        operation = pa_context_set_source_volume_by_index(m_context, device->index(), volume, contextSuccessCallback, this);

    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainLoop);
    pa_operation_unref(operation);

    pa_threaded_mainloop_unlock(m_mainLoop);
}

void PulseAudioEngine::retrieveSinks()
{
    if (!m_ready)
        return;

    pa_threaded_mainloop_lock(m_mainLoop);

    pa_operation *operation;
    operation = pa_context_get_sink_info_list(m_context, sinkInfoCallback, this);
    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainLoop);
    pa_operation_unref(operation);

    pa_threaded_mainloop_unlock(m_mainLoop);
}

void PulseAudioEngine::setupSubscription()
{
    if (!m_ready)
        return;

    connect(this, &PulseAudioEngine::sinkInfoChanged, this, &PulseAudioEngine::retrieveSinkInfo, Qt::QueuedConnection);
    pa_context_set_subscribe_callback(m_context, contextSubscriptionCallback, this);

    pa_threaded_mainloop_lock(m_mainLoop);

    pa_operation *operation;
    operation = pa_context_subscribe(m_context, PA_SUBSCRIPTION_MASK_SINK, contextSuccessCallback, this);
    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainLoop);
    pa_operation_unref(operation);

    pa_threaded_mainloop_unlock(m_mainLoop);
}

void PulseAudioEngine::handleContextStateChanged()
{
    if (m_contextState == PA_CONTEXT_FAILED || m_contextState == PA_CONTEXT_TERMINATED) {
        qWarning("LXQt-Volume: Context connection failed or terminated lets try to reconnect");
        m_reconnectionTimer.start();
    }
}

void PulseAudioEngine::connectContext()
{
    bool keepGoing = true;
    bool ok = false;

    m_reconnectionTimer.stop();

    if (!m_mainLoop)
        return;

    pa_threaded_mainloop_lock(m_mainLoop);

    if (m_context) {
        pa_context_unref(m_context);
        m_context = 0;
    }

    m_context = pa_context_new(m_mainLoopApi, "lxqt-volume");
    pa_context_set_state_callback(m_context, contextStateCallback, this);
    pa_context_set_event_callback(m_context, contextEventCallback, this);

    if (!m_context) {
        pa_threaded_mainloop_unlock(m_mainLoop);
        m_reconnectionTimer.start();
        return;
    }

    if (pa_context_connect(m_context, nullptr, (pa_context_flags_t)0, nullptr) < 0) {
        pa_threaded_mainloop_unlock(m_mainLoop);
        m_reconnectionTimer.start();
        return;
    }

    while (keepGoing) {
        switch (m_contextState) {
            case PA_CONTEXT_CONNECTING:
            case PA_CONTEXT_AUTHORIZING:
            case PA_CONTEXT_SETTING_NAME:
                break;

            case PA_CONTEXT_READY:
                keepGoing = false;
                ok = true;
                break;

            case PA_CONTEXT_TERMINATED:
                keepGoing = false;
                break;

            case PA_CONTEXT_FAILED:
            default:
                qWarning() << QStringLiteral("Connection failure: %1").arg(QString::fromUtf8(pa_strerror(pa_context_errno(m_context))));
                keepGoing = false;
        }

        if (keepGoing)
            pa_threaded_mainloop_wait(m_mainLoop);
    }

    pa_threaded_mainloop_unlock(m_mainLoop);

    if (ok) {
        retrieveSinks();
        setupSubscription();
    } else {
        m_reconnectionTimer.start();
    }
}

void PulseAudioEngine::retrieveSinkInfo(uint32_t idx)
{
    if (!m_ready)
        return;

    pa_threaded_mainloop_lock(m_mainLoop);

    pa_operation *operation;
    operation = pa_context_get_sink_info_by_index(m_context, idx, sinkInfoCallback, this);
    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainLoop);
    pa_operation_unref(operation);

    pa_threaded_mainloop_unlock(m_mainLoop);
}

void PulseAudioEngine::setMute(AudioDevice *device, bool state)
{
    if (!m_ready)
        return;

    pa_threaded_mainloop_lock(m_mainLoop);

    pa_operation *operation;
    operation = pa_context_set_sink_mute_by_index(m_context, device->index(), state, contextSuccessCallback, this);
    while (pa_operation_get_state(operation) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m_mainLoop);
    pa_operation_unref(operation);

    pa_threaded_mainloop_unlock(m_mainLoop);
}

void PulseAudioEngine::setContextState(pa_context_state_t state)
{
    if (m_contextState == state)
        return;

    m_contextState = state;

    // update ready member as it depends on state
    if (m_ready == (m_contextState == PA_CONTEXT_READY))
        return;

    m_ready = (m_contextState == PA_CONTEXT_READY);

    emit contextStateChanged(m_contextState);
    emit readyChanged(m_ready);
}

void PulseAudioEngine::setIgnoreMaxVolume(bool ignore)
{
    if (ignore)
        m_maximumVolume = PA_VOLUME_UI_MAX;
    else
        m_maximumVolume = pa_sw_volume_from_dB(0);
}



