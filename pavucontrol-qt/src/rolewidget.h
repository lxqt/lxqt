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

#ifndef rolewidget_h
#define rolewidget_h

#include "pavucontrol.h"

#include "streamwidget.h"

class RoleWidget : public StreamWidget {
    Q_OBJECT
public:
    RoleWidget(MainWindow* parent);

    QByteArray role;
    QByteArray device;

    virtual void onMuteToggleButton();
    virtual void executeVolumeUpdate();
};

#endif
