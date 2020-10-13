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


#include "dndactionmenu.h"

namespace Fm {

DndActionMenu::DndActionMenu(Qt::DropActions possibleActions, QWidget* parent)
    : QMenu(parent)
    , copyAction(nullptr)
    , moveAction(nullptr)
    , linkAction(nullptr)
    , cancelAction(nullptr) {
    if(possibleActions.testFlag(Qt::CopyAction)) {
        copyAction = addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), tr("Copy here"));
    }
    if(possibleActions.testFlag(Qt::MoveAction)) {
        moveAction = addAction(tr("Move here"));
    }
    if(possibleActions.testFlag(Qt::LinkAction)) {
        linkAction = addAction(tr("Create symlink here"));
    }
    addSeparator();
    cancelAction = addAction(tr("Cancel"));
}

DndActionMenu::~DndActionMenu() {

}

Qt::DropAction DndActionMenu::askUser(Qt::DropActions possibleActions, QPoint pos) {
    Qt::DropAction result = Qt::IgnoreAction;
    DndActionMenu menu{possibleActions};
    QAction* action = menu.exec(pos);
    if(nullptr != action) {
        if(action == menu.copyAction) {
            result = Qt::CopyAction;
        }
        else if(action == menu.moveAction) {
            result = Qt::MoveAction;
        }
        else if(action == menu.linkAction) {
            result = Qt::LinkAction;
        }
    }
    return result;
}


} // namespace Fm
