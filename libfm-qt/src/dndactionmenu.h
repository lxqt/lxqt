/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#ifndef FM_DNDACTIONMENU_H
#define FM_DNDACTIONMENU_H

#include "libfmqtglobals.h"
#include <QMenu>
#include <QAction>

namespace Fm {

class DndActionMenu : public QMenu {
    Q_OBJECT
public:
    explicit DndActionMenu(Qt::DropActions possibleActions, QWidget* parent = nullptr);
    ~DndActionMenu() override;

    static Qt::DropAction askUser(Qt::DropActions possibleActions, QPoint pos);

private:
    QAction* copyAction;
    QAction* moveAction;
    QAction* linkAction;
    QAction* cancelAction;
};

}

#endif // FM_DNDACTIONMENU_H
