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

#include <pulse/ext-device-manager.h>

#include "mainwindow.h"
#include "devicewidget.h"
#include "channel.h"
#include <sstream>
#include <QAction>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>

/*** DeviceWidget ***/
DeviceWidget::DeviceWidget(MainWindow* parent, QByteArray deviceType) :
    MinimalStreamWidget(parent),
    offsetButtonEnabled(false),
    mpMainWindow(parent),
    rename{new QAction{tr("Rename device..."), this}},
    mDeviceType(deviceType) {

    setupUi(this);
    advancedWidget->hide();
    initPeakProgressBar(channelsGrid);

    timeout.setSingleShot(true);
    timeout.setInterval(100);
    connect(&timeout, &QTimer::timeout, this, &DeviceWidget::timeoutEvent);

    connect(muteToggleButton, &QToolButton::toggled, this, &DeviceWidget::onMuteToggleButton);
    connect(lockToggleButton, &QToolButton::toggled, this, &DeviceWidget::onLockToggleButton);
    connect(defaultToggleButton, &QToolButton::toggled, this, &DeviceWidget::onDefaultToggleButton);

    connect(rename, &QAction::triggered, this, &DeviceWidget::renamePopup);
    addAction(rename);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    connect(portList, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &DeviceWidget::onPortChange);
    connect(offsetButton, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &DeviceWidget::onOffsetChange);

    for (auto & channel : channels)
        channel = nullptr;


    // FIXME:
//    offsetAdjustment = Gtk::Adjustment::create(0.0, -2000.0, 2000.0, 10.0, 50.0, 0.0);
//    offsetButton->configure(offsetAdjustment, 0, 2);
}

void DeviceWidget::setChannelMap(const pa_channel_map &m, bool can_decibel) {
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

    lockToggleButton->setEnabled(m.channels > 1);
    hideLockedChannels(lockToggleButton->isChecked());
}

void DeviceWidget::setVolume(const pa_cvolume &v, bool force) {
    g_assert(v.channels == channelMap.channels);

    volume = v;

    if (!timeout.isActive() || force) { /* do not update the volume when a volume change is still in flux */
        for (int i = 0; i < volume.channels; i++)
            channels[i]->setVolume(volume.values[i]);
    }
}

void DeviceWidget::updateChannelVolume(int channel, pa_volume_t v) {
    pa_cvolume n;
    g_assert(channel < volume.channels);

    n = volume;
    if (lockToggleButton->isChecked())
        pa_cvolume_set(&n, n.channels, v);
    else
        n.values[channel] = v;

    setVolume(n, true);

    if (!timeout.isActive()) {
        timeout.start();
    }
}

void DeviceWidget::hideLockedChannels(bool hide) {
    for (int i = 0; i < channelMap.channels - 1; i++)
        channels[i]->setVisible(!hide);

    channels[channelMap.channels - 1]->channelLabel->setVisible(!hide);
}

void DeviceWidget::onMuteToggleButton() {

    lockToggleButton->setEnabled(!muteToggleButton->isChecked());

    for (int i = 0; i < channelMap.channels; i++)
        channels[i]->setEnabled(!muteToggleButton->isChecked());
}

void DeviceWidget::onLockToggleButton() {
    hideLockedChannels(lockToggleButton->isChecked());
}

void DeviceWidget::onDefaultToggleButton() {
}

void DeviceWidget::onOffsetChange() {
    pa_operation *o;
    int64_t offset;
    std::ostringstream card_stream;
    QByteArray card_name;

    if (!offsetButtonEnabled)
        return;

    offset = offsetButton->value() * 1000.0;
    card_stream << card_index;
    card_name = QByteArray::fromStdString(card_stream.str());

    if (!(o = pa_context_set_port_latency_offset(get_context(),
            card_name.constData(), activePort.constData(), offset, nullptr, nullptr))) {
        show_error(tr("pa_context_set_port_latency_offset() failed").toUtf8().constData());
        return;
    }
    pa_operation_unref(o);
}

void DeviceWidget::setDefault(bool isDefault) {
    defaultToggleButton->setChecked(isDefault);
    /*defaultToggleButton->setEnabled(!isDefault);*/
}

bool DeviceWidget::timeoutEvent() {
    executeVolumeUpdate();
    return false;
}

void DeviceWidget::executeVolumeUpdate() {
}

void DeviceWidget::setLatencyOffset(int64_t offset) {
    offsetButtonEnabled = false;
    offsetButton->setValue(offset / 1000.0);
    offsetButtonEnabled = true;
}

void DeviceWidget::setBaseVolume(pa_volume_t v) {

    for (int i = 0; i < channelMap.channels; i++)
        channels[i]->setBaseVolume(v);
}

void DeviceWidget::prepareMenu() {
    int idx = 0;
    int active_idx = -1;

    portList->clear();
    /* Fill the ComboBox's Model */
    for (auto & port : ports) {
        QByteArray name = port.first;
        QString desc = QString::fromUtf8(port.second);
        portList->addItem(desc, name);
        if (port.first == activePort)
            active_idx = idx;
        idx++;
    }

    if (active_idx >= 0)
        portList->setCurrentIndex(active_idx);

    if (!ports.empty()) {
        portSelect->show();

        if (pa_context_get_server_protocol_version(get_context()) >= 27) {
            offsetSelect->show();
            advancedOptions->setEnabled(true);
        } else {
            /* advancedOptions has sensitive=false by default */
            offsetSelect->hide();
        }

    } else {
        portSelect->hide();
        advancedOptions->setEnabled(false);
        offsetSelect->hide();
    }
}

void DeviceWidget::renamePopup() {
    if (updating)
        return;
    if (!mpMainWindow->canRenameDevices) {
        QMessageBox::warning(this, tr("Sorry, but device renaming is not supported.")
                , tr("You need to load module-device-manager in the PulseAudio server in order to rename devices"));
        return;
    }

    const QString old_name = QString::fromUtf8(description);
    bool ok;
    const QString new_name = QInputDialog::getText(this, QCoreApplication::organizationName(), tr("Rename device %1 to:").arg(old_name)
            , QLineEdit::Normal, old_name, &ok);
    if (ok && new_name != old_name) {
        pa_operation* o;
        gchar *key = g_markup_printf_escaped("%s:%s", mDeviceType.constData(), name.constData());

        if (!(o = pa_ext_device_manager_set_device_description(get_context(), key, new_name.toUtf8().constData(), nullptr, nullptr)))
            show_error(tr("pa_ext_device_manager_set_device_description() failed").toUtf8().constData());
        else
            pa_operation_unref(o);
        g_free(key);
    }
}
