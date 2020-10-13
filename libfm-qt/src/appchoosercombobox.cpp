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

#include "appchoosercombobox.h"
#include "appchooserdialog.h"
#include "utilities.h"
#include "core/iconinfo.h"

namespace Fm {

AppChooserComboBox::AppChooserComboBox(QWidget* parent):
    QComboBox(parent),
    defaultAppIndex_(-1),
    prevIndex_(0),
    blockOnCurrentIndexChanged_(false) {

    // the new Qt5 signal/slot syntax cannot handle overloaded methods by default
    // hence a type-casting is needed here. really ugly!
    // reference: https://forum.qt.io/topic/20998/qt5-new-signals-slots-syntax-does-not-work-solved
    connect((QComboBox*)this, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &AppChooserComboBox::onCurrentIndexChanged);
}

AppChooserComboBox::~AppChooserComboBox() {
}

void AppChooserComboBox::setMimeType(std::shared_ptr<const Fm::MimeType> mimeType) {
    clear();
    defaultApp_.reset();
    appInfos_.clear();

    mimeType_ = std::move(mimeType);
    if(mimeType_) {
        const char* typeName = mimeType_->name();
        defaultApp_ = Fm::GAppInfoPtr{g_app_info_get_default_for_type(typeName, FALSE), false};
        GList* appInfos_glist = g_app_info_get_all_for_type(typeName);
        int i = 0;
        for(GList* l = appInfos_glist; l; l = l->next, ++i) {
            Fm::GAppInfoPtr app{G_APP_INFO(l->data), false};
            GIcon* gicon = g_app_info_get_icon(app.get());
            addItem(gicon ? Fm::IconInfo::fromGIcon(gicon)->qicon(): QIcon(), QString::fromUtf8(g_app_info_get_name(app.get())));
            if(g_app_info_equal(app.get(), defaultApp_.get())) {
                defaultAppIndex_ = i;
            }
            appInfos_.push_back(std::move(app));
        }
        g_list_free(appInfos_glist);
    }
    // add "Other applications" item
    insertSeparator(count());
    addItem(tr("Customize"));
    if(defaultAppIndex_ != -1) {
        setCurrentIndex(defaultAppIndex_);
    }
}

// returns the currently selected app.
Fm::GAppInfoPtr AppChooserComboBox::selectedApp() const {
    // the elements of appInfos_ and the combo indexes before "Customize"
    // always have a one-to-one correspondence
    int idx = currentIndex();
    return idx >= 0 && !appInfos_.empty() ? appInfos_[idx] : Fm::GAppInfoPtr{};
}

bool AppChooserComboBox::isChanged() const {
    return (defaultAppIndex_ != currentIndex());
}

void AppChooserComboBox::onCurrentIndexChanged(int index) {
    if(index == -1 || index == prevIndex_ || blockOnCurrentIndexChanged_) {
        return;
    }

    // the last item is "Customize"
    if(index == (count() - 1)) {
        /* TODO: let the user choose an app or add custom actions here. */
        QWidget* toplevel = topLevelWidget();
        AppChooserDialog dlg(mimeType_, toplevel);
        dlg.setWindowModality(Qt::WindowModal);
        dlg.setCanSetDefault(false);
        if(dlg.exec() == QDialog::Accepted) {
            auto app = dlg.selectedApp();
            if(app) {
                /* see if it's already in the list to prevent duplication */
                auto found = std::find_if(appInfos_.cbegin(), appInfos_.cend(), [&](const Fm::GAppInfoPtr& item) {
                    return g_app_info_equal(app.get(), item.get());
                });

                // inserting new items or change current index will recursively trigger onCurrentIndexChanged.
                // we need to block our handler to prevent recursive calls.
                blockOnCurrentIndexChanged_ = true;
                /* if it's already in the list, select it */
                if(found != appInfos_.cend()) {
                    auto pos = found - appInfos_.cbegin();
                    setCurrentIndex(pos);
                }
                else { /* if it's not found, add it to the list */
                    auto it = appInfos_.insert(appInfos_.cbegin(), std::move(app));
                    GIcon* gicon = g_app_info_get_icon(it->get());
                    insertItem(0, Fm::IconInfo::fromGIcon(gicon)->qicon(), QString::fromUtf8(g_app_info_get_name(it->get())));
                    setCurrentIndex(0);
                }
                blockOnCurrentIndexChanged_ = false;
                return;
            }
        }

        // block our handler to prevent recursive calls.
        blockOnCurrentIndexChanged_ = true;
        // restore to previously selected item
        setCurrentIndex(prevIndex_);
        blockOnCurrentIndexChanged_ = false;
    }
    else {
        prevIndex_ = index;
    }
}


#if 0
/* get a list of custom apps added with app-chooser.
* the returned GList is owned by the combo box and shouldn't be freed. */
const GList* AppChooserComboBox::customApps() {

}
#endif

} // namespace Fm
