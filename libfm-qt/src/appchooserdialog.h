/*
 * Copyright 2010-2014 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifndef FM_APPCHOOSERDIALOG_H
#define FM_APPCHOOSERDIALOG_H

#include <QDialog>
#include "libfmqtglobals.h"

#include "core/mimetype.h"
#include "core/gioptrs.h"

namespace Ui {
class AppChooserDialog;
}

namespace Fm {

class LIBFM_QT_API AppChooserDialog : public QDialog {
    Q_OBJECT
public:
    explicit AppChooserDialog(std::shared_ptr<const Fm::MimeType> mimeType, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~AppChooserDialog() override;

    void accept() override;

    void setMimeType(std::shared_ptr<const Fm::MimeType> mimeType);

    const std::shared_ptr<const Fm::MimeType>& mimeType() const {
        return mimeType_;
    }

    void setCanSetDefault(bool value);

    bool canSetDefault() const {
        return canSetDefault_;
    }

    const Fm::GAppInfoPtr& selectedApp() const {
        return selectedApp_;
    }

    bool isSetDefault() const;

private:
    GAppInfo* customCommandToApp();

private Q_SLOTS:
    void onSelectionChanged();
    void onTabChanged(int index);

private:
    Ui::AppChooserDialog* ui;
    std::shared_ptr<const Fm::MimeType> mimeType_;
    bool canSetDefault_;
    Fm::GAppInfoPtr selectedApp_;
};

}

#endif // FM_APPCHOOSERDIALOG_H
