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

#include "sourcewidget.h"

SourceWidget::SourceWidget(MainWindow *parent) :
    DeviceWidget(parent, "source") {
}

void SourceWidget::executeVolumeUpdate() {
    pa_operation* o;

    if (!(o = pa_context_set_source_volume_by_index(get_context(), index, &volume, nullptr, nullptr))) {
        show_error(tr("pa_context_set_source_volume_by_index() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void SourceWidget::onMuteToggleButton() {
    DeviceWidget::onMuteToggleButton();

    if (updating)
        return;

    pa_operation* o;
    if (!(o = pa_context_set_source_mute_by_index(get_context(), index, muteToggleButton->isChecked(), nullptr, nullptr))) {
        show_error(tr("pa_context_set_source_mute_by_index() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void SourceWidget::onDefaultToggleButton() {
    pa_operation* o;

    if (updating)
        return;

    if (!(o = pa_context_set_default_source(get_context(), name.constData(), nullptr, nullptr))) {
        show_error(tr("pa_context_set_default_source() failed").toUtf8().constData());
        return;
    }
    pa_operation_unref(o);
}

void SourceWidget::onPortChange() {
    if (updating)
        return;

    int current = portList->currentIndex();
    if (current != -1) {
        pa_operation* o;
        QByteArray port = portList->itemData(current).toByteArray();

        if (!(o = pa_context_set_source_port_by_index(get_context(), index, port.constData(), nullptr, nullptr))) {
            show_error(tr("pa_context_set_source_port_by_index() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
    }
}
