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

#include "sinkinputwidget.h"
#include "mainwindow.h"
#include "sinkwidget.h"
#include <QMenu>


SinkInputWidget::SinkInputWidget(MainWindow *parent) :
    StreamWidget(parent),
    menu{new QMenu{this}} {

    gchar *txt;
    directionLabel->setText(QString::fromUtf8(txt = g_markup_printf_escaped("<i>%s</i>", tr("on").toUtf8().constData())));
    g_free(txt);

    terminate->setText(tr("Terminate Playback"));
}

SinkInputWidget::~SinkInputWidget(void) {
}

void SinkInputWidget::setSinkIndex(uint32_t idx) {
    mSinkIndex = idx;

    if (mpMainWindow->sinkWidgets.count(idx)) {
        SinkWidget *w = mpMainWindow->sinkWidgets[idx];
        deviceButton->setText(QString::fromUtf8(w->description));
    }
    else
        deviceButton->setText(tr("Unknown output"));
}

uint32_t SinkInputWidget::sinkIndex() {
    return mSinkIndex;
}

void SinkInputWidget::executeVolumeUpdate() {
    pa_operation* o;

    if (!(o = pa_context_set_sink_input_volume(get_context(), index, &volume, nullptr, nullptr))) {
        show_error(tr("pa_context_set_sink_input_volume() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void SinkInputWidget::onMuteToggleButton() {
    StreamWidget::onMuteToggleButton();

    if (updating)
        return;

    pa_operation* o;
    if (!(o = pa_context_set_sink_input_mute(get_context(), index, muteToggleButton->isChecked(), nullptr, nullptr))) {
        show_error(tr("pa_context_set_sink_input_mute() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void SinkInputWidget::onKill() {
    pa_operation* o;
    if (!(o = pa_context_kill_sink_input(get_context(), index, nullptr, nullptr))) {
        show_error(tr("pa_context_kill_sink_input() failed").toUtf8().constData());
        return;
    }

    pa_operation_unref(o);
}

void SinkInputWidget::buildMenu() {
  for (auto & sinkWidget : mpMainWindow->sinkWidgets) {
      menu->addAction(new SinkMenuItem{this, sinkWidget.second->description.constData(), sinkWidget.second->index, sinkWidget.second->index == mSinkIndex, menu});
  }
}

void SinkInputWidget::SinkMenuItem::onToggle() {
  if (widget->updating)
    return;

  if (!isChecked())
    return;

  /*if (!mpMainWindow->sinkWidgets.count(widget->index))
    return;*/

  pa_operation* o;
  if (!(o = pa_context_move_sink_input_by_index(get_context(), widget->index, index, nullptr, nullptr))) {
    show_error(tr("pa_context_move_sink_input_by_index() failed").toUtf8().constData());
    return;
  }

  pa_operation_unref(o);
}

void SinkInputWidget::onDeviceChangePopup() {
    menu->clear();
    buildMenu();
    menu->popup(QCursor::pos());
}
