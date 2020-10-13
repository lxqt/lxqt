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

#include "sinkwidget.h"

// #include <canberra-gtk.h>
#if HAVE_EXT_DEVICE_RESTORE_API
#  include <pulse/format.h>
#  include <pulse/ext-device-restore.h>
#endif

SinkWidget::SinkWidget(MainWindow *parent) :
    DeviceWidget(parent, "sink") {

#if HAVE_EXT_DEVICE_RESTORE_API
    uint8_t i = 0;

    encodings[i].encoding = PA_ENCODING_PCM;
    encodings[i].widget = encodingFormatPCM;
    connect(encodings[i].widget, &QCheckBox::toggled, this, &SinkWidget::onEncodingsChange);

    ++i;
    encodings[i].encoding = PA_ENCODING_AC3_IEC61937;
    encodings[i].widget = encodingFormatAC3;
    connect(encodings[i].widget, &QCheckBox::toggled, this, &SinkWidget::onEncodingsChange);

    ++i;
    encodings[i].encoding = PA_ENCODING_EAC3_IEC61937;
    encodings[i].widget = encodingFormatEAC3;
    connect(encodings[i].widget, &QCheckBox::toggled, this, &SinkWidget::onEncodingsChange);

    ++i;
    encodings[i].encoding = PA_ENCODING_MPEG_IEC61937;
    encodings[i].widget = encodingFormatMPEG;
    connect(encodings[i].widget, &QCheckBox::toggled, this, &SinkWidget::onEncodingsChange);

    ++i;
    encodings[i].encoding = PA_ENCODING_DTS_IEC61937;
    encodings[i].widget = encodingFormatDTS;
    connect(encodings[i].widget, &QCheckBox::toggled, this, &SinkWidget::onEncodingsChange);

    ++i;
    encodings[i].encoding = PA_ENCODING_INVALID;
    encodings[i].widget = encodingFormatAAC;
    encodings[i].widget->setEnabled(false);
#ifdef PA_ENCODING_MPEG2_AAC_IEC61937
    if (pa_context_get_server_protocol_version(get_context()) >= 28) {
        encodings[i].encoding = PA_ENCODING_MPEG2_AAC_IEC61937;
        connect(encodings[i].widget, &QCheckBox::toggled, this, &SinkWidget::onEncodingsChange);
        encodings[i].widget->setEnabled(true);
    }
#endif
#endif

}

void SinkWidget::executeVolumeUpdate() {
    pa_operation* o;
    if (!(o = pa_context_set_sink_volume_by_index(get_context(), index, &volume, nullptr, nullptr))) {
        show_error(tr("pa_context_set_sink_volume_by_index() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void SinkWidget::onMuteToggleButton() {
    DeviceWidget::onMuteToggleButton();

    if (updating)
        return;

    pa_operation* o;
    if (!(o = pa_context_set_sink_mute_by_index(get_context(), index, muteToggleButton->isChecked(), nullptr, nullptr))) {
        show_error(tr("pa_context_set_sink_mute_by_index() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void SinkWidget::onDefaultToggleButton() {
    pa_operation* o;

    if (updating)
        return;

    if (!(o = pa_context_set_default_sink(get_context(), name.constData(), nullptr, nullptr))) {
        show_error(tr("pa_context_set_default_sink() failed").toUtf8().constData());
        return;
    }
    pa_operation_unref(o);
}

void SinkWidget::onPortChange() {
    if (updating)
        return;

    int sel = portList->currentIndex();
    if (sel != -1) {
        pa_operation* o;
        QByteArray port = portList->itemData(sel).toString().toUtf8();

        if (!(o = pa_context_set_sink_port_by_index(get_context(), index, port.constData(), nullptr, nullptr))) {
            show_error(tr("pa_context_set_sink_port_by_index() failed").toUtf8().constData());
            return;
        }

        pa_operation_unref(o);
    }
}

void SinkWidget::setDigital(bool digital) {
#if HAVE_EXT_DEVICE_RESTORE_API
    if (digital) {
        encodingSelect->show();
        advancedOptions->setEnabled(true);
    } else {
        /* advancedOptions is disabled by default */
        encodingSelect->hide();
    }
#endif
}

void SinkWidget::onEncodingsChange() {
#if HAVE_EXT_DEVICE_RESTORE_API
    pa_operation* o;
    uint8_t n_formats = 0;
    pa_format_info **formats;

    if (updating)
        return;

    formats = (pa_format_info**)malloc(sizeof(pa_format_info*) * PAVU_NUM_ENCODINGS);

    for (auto & encoding : encodings) {
        if (encoding.widget->isChecked()) {
            formats[n_formats] = pa_format_info_new();
            formats[n_formats]->encoding = encoding.encoding;
            ++n_formats;
        }
    }

    if (!(o = pa_ext_device_restore_save_formats(get_context(), PA_DEVICE_TYPE_SINK, index, n_formats, formats, nullptr, nullptr))) {
        show_error(tr("pa_ext_device_restore_save_sink_formats() failed").toUtf8().constData());
        free(formats);
        return;
    }

    free(formats);
    pa_operation_unref(o);
#endif
}
