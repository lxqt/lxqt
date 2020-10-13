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

#include "rolewidget.h"

#include <pulse/ext-stream-restore.h>

RoleWidget::RoleWidget(MainWindow *parent) :
    StreamWidget(parent) {

    lockToggleButton->hide();
    directionLabel->hide();
    deviceButton->hide();
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void RoleWidget::onMuteToggleButton() {
    StreamWidget::onMuteToggleButton();

    executeVolumeUpdate();
}

void RoleWidget::executeVolumeUpdate() {
    pa_ext_stream_restore_info info;

    if (updating)
        return;

    info.name = role.constData();
    info.channel_map.channels = 1;
    info.channel_map.map[0] = PA_CHANNEL_POSITION_MONO;
    info.volume = volume;
    info.device = device == "" ? nullptr : device.constData();
    info.mute = muteToggleButton->isChecked();

    pa_operation* o;
    if (!(o = pa_ext_stream_restore_write(get_context(), PA_UPDATE_REPLACE, &info, 1, TRUE, nullptr, nullptr))) {
        show_error(tr("pa_ext_stream_restore_write() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

