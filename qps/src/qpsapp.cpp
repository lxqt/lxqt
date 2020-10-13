/*
 * qpsapp.cpp
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

#include "qpsapp.h"

void QpsApp::saveState(QSessionManager &/*manager*/)
{
    //	printf("saveState()\n");
    // manager.setRestartHint(QSessionManager::RestartIfRunning);
    // manager.release();
}

// this is called  when X Logout
// closeEvent() never called !!
void QpsApp::commitData(QSessionManager &/*manager*/)
{
    /*
    printf("commitData()\n");
    manager.setRestartHint(QSessionManager::RestartIfRunning);
    qps->flag_exit=true;  // ready to Logout
    qps->save_settings() ;
    manager.release();
    sleep(2);
    return;
     if (manager.allowsInteraction()) {
     int ret = QMessageBox::warning(
                 qps,
                 tr("My Application"),
                 tr("Save changes to document?"),
                 QMessageBox::Save | QMessageBox::Discard |
  QMessageBox::Cancel);

     switch (ret) {
     case QMessageBox::Save:
         manager.release();
  //          if (!saveDocument())    manager.cancel();
         break;
     case QMessageBox::Discard:
         break;
     case QMessageBox::Cancel:
     default:
         manager.cancel();
     }
  } else {

                    manager.release();

     // we did not get permission to interact, then
     // do something reasonable instead
  }
  */
    /*
  //DEL sm.release();
    qDebug("Qps: Session saved\n");
  //	sm.cancel();
    //sm.setRestartHint (QSessionManager::RestartIfRunning);
    QApplication::commitData(sm);
  */
}
