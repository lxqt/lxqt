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

#ifndef sinkinputwidget_h
#define sinkinputwidget_h

#include "pavucontrol.h"

#include "streamwidget.h"
#include <QAction>

class MainWindow;
class QMenu;

class SinkInputWidget : public StreamWidget {
    Q_OBJECT
public:
    SinkInputWidget(MainWindow *parent);
    ~SinkInputWidget(void);

    SinkInputType type;

    uint32_t index, clientIndex;
    void setSinkIndex(uint32_t idx);
    uint32_t sinkIndex();
    virtual void executeVolumeUpdate();
    virtual void onMuteToggleButton();
    virtual void onDeviceChangePopup();
    virtual void onKill();

private:
    uint32_t mSinkIndex;

    void buildMenu();

    QMenu * menu;

    struct SinkMenuItem : public QAction
    {
        SinkMenuItem(SinkInputWidget *w
                , const char *label
                , uint32_t i
                , bool active
                , QObject * parent = nullptr)
            : QAction(QString::fromUtf8(label), parent)
            , widget(w)
            , index(i)
        {
            setCheckable(true);
            setChecked(active);
            connect(this, &QAction::toggled, [this] { onToggle(); });
        }

        SinkInputWidget *widget;
        uint32_t index;
        void onToggle();
    };
};

#endif
