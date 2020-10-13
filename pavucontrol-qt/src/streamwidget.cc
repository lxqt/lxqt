/***
  This file is part of pavucontrol.

  Copyright 2006-2008 Lennart Poettering
  Copyright 2009 Colin Guthrie

  pavucontrol is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  pavucontrol is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with pavucontrol. If not, see <https://www.gnu.org/licenses/>.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "streamwidget.h"
#include "mainwindow.h"
#include "channel.h"
#include <QAction>

/*** StreamWidget ***/
StreamWidget::StreamWidget(MainWindow *parent) :
    MinimalStreamWidget(parent),
    mpMainWindow(parent),
    terminate{new QAction{tr("Terminate"), this}} {

    setupUi(this);
    initPeakProgressBar(channelsGrid);

    timeout.setSingleShot(true);
    timeout.setInterval(100);
    connect(&timeout, &QTimer::timeout, this, &StreamWidget::timeoutEvent);

    connect(muteToggleButton, &QToolButton::toggled, this, &StreamWidget::onMuteToggleButton);
    connect(lockToggleButton, &QToolButton::toggled, this, &StreamWidget::onLockToggleButton);
    connect(deviceButton, &QAbstractButton::released, this, &StreamWidget::onDeviceChangePopup);

    connect(terminate, &QAction::triggered, this, &StreamWidget::onKill);
    addAction(terminate);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    for (auto & channel : channels)
        channel = nullptr;
}

void StreamWidget::setChannelMap(const pa_channel_map &m, bool can_decibel) {
    channelMap = m;

    for (int i = 0; i < m.channels; i++) {
        Channel *ch = channels[i] = new Channel(channelsGrid);
        ch->channel = i;
        ch->can_decibel = can_decibel;
        ch->minimalStreamWidget = this;
        char text[64];
        snprintf(text, sizeof(text), "<b>%s</b>", pa_channel_position_to_pretty_string(m.map[i]));
        ch->channelLabel->setText(QString::fromUtf8(text));
    }
    channels[m.channels-1]->last = true;
    channels[m.channels-1]->setBaseVolume(PA_VOLUME_NORM);

    lockToggleButton->setEnabled(m.channels > 1);
    hideLockedChannels(lockToggleButton->isChecked());
}

void StreamWidget::setVolume(const pa_cvolume &v, bool force) {
    g_assert(v.channels == channelMap.channels);

    volume = v;

    if (!timeout.isActive() || force) { /* do not update the volume when a volume change is still in flux */
        for (int i = 0; i < volume.channels; i++)
            channels[i]->setVolume(volume.values[i]);
    }
}

void StreamWidget::updateChannelVolume(int channel, pa_volume_t v) {
    pa_cvolume n;
    g_assert(channel < volume.channels);

    n = volume;
    if (lockToggleButton->isChecked()) {
        for (int i = 0; i < n.channels; i++)
            n.values[i] = v;
    } else
        n.values[channel] = v;

    setVolume(n, true);

    if(!timeout.isActive()) {
        timeout.start();
    }
}

void StreamWidget::hideLockedChannels(bool hide) {
    for (int i = 0; i < channelMap.channels - 1; i++)
        channels[i]->setVisible(!hide);

    channels[channelMap.channels - 1]->channelLabel->setVisible(!hide);
}

void StreamWidget::onMuteToggleButton() {

    lockToggleButton->setEnabled(!muteToggleButton->isChecked());

    for (int i = 0; i < channelMap.channels; i++)
        channels[i]->setEnabled(!muteToggleButton->isChecked());
}

void StreamWidget::onLockToggleButton() {
    hideLockedChannels(lockToggleButton->isChecked());
}

bool StreamWidget::timeoutEvent() {
    executeVolumeUpdate();
    return false;
}

void StreamWidget::executeVolumeUpdate() {
}

void StreamWidget::onDeviceChangePopup() {
}

void StreamWidget::onKill() {
}
