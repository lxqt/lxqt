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

#ifndef FM_FILEMENU_P_H
#define FM_FILEMENU_P_H

#include <QDebug>
#include "core/gioptrs.h"
#include "core/iconinfo.h"

namespace Fm {

class AppInfoAction : public QAction {
    Q_OBJECT
public:
    explicit AppInfoAction(Fm::GAppInfoPtr app, QObject* parent = nullptr):
        QAction(QString::fromUtf8(g_app_info_get_name(app.get())), parent),
        appInfo_{std::move(app)} {
        setToolTip(QString::fromUtf8(g_app_info_get_description(appInfo_.get())));
        GIcon* gicon = g_app_info_get_icon(appInfo_.get());
        const auto icnInfo = Fm::IconInfo::fromGIcon(gicon);
        if(icnInfo) {
            setIcon(icnInfo->qicon());
        }
    }

    ~AppInfoAction() override {
    }

    const Fm::GAppInfoPtr& appInfo() const {
        return appInfo_;
    }

private:
    Fm::GAppInfoPtr appInfo_;
};

} // namespace Fm

#endif
