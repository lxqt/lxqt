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

#ifndef mainwindow_h
#define mainwindow_h

#include "pavucontrol.h"
#include <pulse/ext-stream-restore.h>
#if HAVE_EXT_DEVICE_RESTORE_API
#  include <pulse/ext-device-restore.h>
#endif

#include <QDialog>
#include "ui_mainwindow.h"

class CardWidget;
class SinkWidget;
class SourceWidget;
class SinkInputWidget;
class SourceOutputWidget;
class RoleWidget;

class MainWindow : public QDialog, public Ui::MainWindow {
    Q_OBJECT
public:
    MainWindow();
    virtual ~MainWindow();

    void updateCard(const pa_card_info &info);
    bool updateSink(const pa_sink_info &info);
    void updateSource(const pa_source_info &info);
    void updateSinkInput(const pa_sink_input_info &info);
    void updateSourceOutput(const pa_source_output_info &info);
    void updateClient(const pa_client_info &info);
    void updateServer(const pa_server_info &info);
    void updateVolumeMeter(uint32_t source_index, uint32_t sink_input_index, double v);
    void updateRole(const pa_ext_stream_restore_info &info);
#if HAVE_EXT_DEVICE_RESTORE_API
    void updateDeviceInfo(const pa_ext_device_restore_info &info);
#endif

    void removeCard(uint32_t index);
    void removeSink(uint32_t index);
    void removeSource(uint32_t index);
    void removeSinkInput(uint32_t index);
    void removeSourceOutput(uint32_t index);
    void removeClient(uint32_t index);

    void removeAllWidgets();

    void setConnectingMessage(const char *string = NULL);

    std::map<uint32_t, CardWidget*> cardWidgets;
    std::map<uint32_t, SinkWidget*> sinkWidgets;
    std::map<uint32_t, SourceWidget*> sourceWidgets;
    std::map<uint32_t, SinkInputWidget*> sinkInputWidgets;
    std::map<uint32_t, SourceOutputWidget*> sourceOutputWidgets;

    std::map<uint32_t, char*> clientNames;
    SinkInputType showSinkInputType;
    SinkType showSinkType;
    SourceOutputType showSourceOutputType;
    SourceType showSourceType;

protected Q_SLOTS:
    virtual void onSinkInputTypeComboBoxChanged(int index);
    virtual void onSourceOutputTypeComboBoxChanged(int index);
    virtual void onSinkTypeComboBoxChanged(int index);
    virtual void onSourceTypeComboBoxChanged(int index);
    virtual void onShowVolumeMetersCheckButtonToggled(bool toggled);

public:
    void setConnectionState(gboolean connected);
    void updateDeviceVisibility();
    void reallyUpdateDeviceVisibility();
    pa_stream* createMonitorStreamForSource(uint32_t source_idx, uint32_t stream_idx, bool suspend);
    void createMonitorStreamForSinkInput(SinkInputWidget* w, uint32_t sink_idx);

    void setIconFromProplist(QLabel *icon, pa_proplist *l, const char *name);

    RoleWidget *eventRoleWidget;

    bool createEventRoleWidget();
    void deleteEventRoleWidget();

    QByteArray defaultSinkName, defaultSourceName;

    bool canRenameDevices;

private:
    gboolean m_connected;
    gchar* m_config_filename;
};


#endif
