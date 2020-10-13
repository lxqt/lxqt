/*
 * qpsapp.h
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 * Copyright 2005-2012 fasthyun@magicn.com
 * Copyright 2015-     daehyun.yang@gmail.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef QPSAPP_H
#define QPSAPP_H

#include <LXQt/SingleApplication>

// MOD!!!: For systray update.
// this trick very suck, but I can't find a better solution.
class QpsApp : public LXQt::SingleApplication
{
  public:
    QpsApp(int &argc, char **argv) : LXQt::SingleApplication(argc, argv){};
    void commitData(QSessionManager &sm);
    void saveState(QSessionManager &manager);

    /*
        virtual bool x11EventFilter ( XEvent *xev ){
                // catch X11 event for systray_update !! which event?
                ///if(trayicon!=NULL) return
       trayicon->checkNewTrayEvent(xev);
                return false; // events to qt.
        }; */
};

#endif // QPSAPP_H
