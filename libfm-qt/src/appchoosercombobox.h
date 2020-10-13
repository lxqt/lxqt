/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef FM_APPCHOOSERCOMBOBOX_H
#define FM_APPCHOOSERCOMBOBOX_H

#include "libfmqtglobals.h"
#include <QComboBox>

#include <vector>

#include "core/mimetype.h"
#include "core/gioptrs.h"

namespace Fm {

class LIBFM_QT_API AppChooserComboBox : public QComboBox {
    Q_OBJECT
public:
    ~AppChooserComboBox() override;
    explicit AppChooserComboBox(QWidget* parent);

    void setMimeType(std::shared_ptr<const Fm::MimeType> mimeType);

    const std::shared_ptr<const Fm::MimeType>& mimeType() const {
        return mimeType_;
    }

    Fm::GAppInfoPtr selectedApp() const;
    // const GList* customApps();

    bool isChanged() const;

private Q_SLOTS:
    void onCurrentIndexChanged(int index);

private:
    std::shared_ptr<const Fm::MimeType> mimeType_;
    std::vector<Fm::GAppInfoPtr> appInfos_; // applications used to open the file type
    Fm::GAppInfoPtr defaultApp_; // default application used to open the file type
    int defaultAppIndex_;
    int prevIndex_;
    bool blockOnCurrentIndexChanged_;
};

}

#endif // FM_APPCHOOSERCOMBOBOX_H
