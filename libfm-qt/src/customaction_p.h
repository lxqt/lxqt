/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef FM_CUSTOMACTION_P_H
#define FM_CUSTOMACTION_P_H

#include <QAction>
#include "customactions/fileaction.h"

namespace Fm {

class CustomAction : public QAction {
public:
    explicit CustomAction(std::shared_ptr<const FileActionItem> item, QObject* parent = nullptr):
        QAction{QString::fromStdString(item->get_name()), parent},
        item_{item} {
        auto& icon_name = item->get_icon();
        if(!icon_name.empty()) {
            setIcon(QIcon::fromTheme(QString::fromUtf8(icon_name.c_str())));
        }
    }

    ~CustomAction() override {
    }

    const std::shared_ptr<const FileActionItem>& item() const {
        return item_;
    }

private:
    std::shared_ptr<const FileActionItem> item_;
};

} // namespace Fm

#endif
