/*

    Copyright (C) 2013  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <QPushButton>
#include "autorundialog.h"
#include <QListWidgetItem>
#include "application.h"
#include "mainwindow.h"
#include <libfm-qt/core/filepath.h>
#include <libfm-qt/core/iconinfo.h>

namespace PCManFM {

AutoRunDialog::AutoRunDialog(GVolume* volume, GMount* mount, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    cancellable(g_cancellable_new()),
    applications(nullptr),
    mount_(G_MOUNT(g_object_ref(mount))) {

    setAttribute(Qt::WA_DeleteOnClose);
    ui.setupUi(this);
    ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    GIcon* gicon = g_volume_get_icon(volume);
    QIcon icon = Fm::IconInfo::fromGIcon(gicon)->qicon();
    ui.icon->setPixmap(icon.pixmap(QSize(48, 48)));
    // add actions
    QListWidgetItem* item = new QListWidgetItem(QIcon::fromTheme(QStringLiteral("system-file-manager")), tr("Open in file manager"));
    ui.listWidget->addItem(item);

    g_mount_guess_content_type(mount, TRUE, cancellable, (GAsyncReadyCallback)onContentTypeFinished, this);

    connect(ui.listWidget, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
}

AutoRunDialog::~AutoRunDialog() {
    g_list_foreach(applications, (GFunc)g_object_unref, nullptr);
    g_list_free(applications);

    if(mount_) {
        g_object_unref(mount_);
    }

    if(cancellable) {
        g_cancellable_cancel(cancellable);
        g_object_unref(cancellable);
    }
}

void AutoRunDialog::accept() {
    QListWidgetItem* item = ui.listWidget->selectedItems().first();
    if(item) {
        GFile* gf = g_mount_get_root(mount_);
        void* p = item->data(Qt::UserRole).value<void*>();
        if(p) { // run the selected application
            GAppInfo* app = G_APP_INFO(p);
            GList* filelist = g_list_prepend(nullptr, gf);
            g_app_info_launch(app, filelist, nullptr, nullptr);
            g_list_free(filelist);
        }
        else {
            // the default action, open the mounted folder in the file manager
            Application* app = static_cast<Application*>(qApp);
            Settings& settings = app->settings();
            Fm::FilePath path{gf, true};
            // open the path in a new window
            // FIXME: or should we open it in a new tab? Make this optional later
            MainWindow* win = new MainWindow(path);
            win->resize(settings.windowWidth(), settings.windowHeight());
            if(settings.windowMaximized()) {
                win->setWindowState(win->windowState() | Qt::WindowMaximized);
            }
            win->show();
        }
        g_object_unref(gf);
    }
    QDialog::accept();
}

// static
void AutoRunDialog::onContentTypeFinished(GMount* mount, GAsyncResult* res, AutoRunDialog* pThis) {
    if(pThis->cancellable) {
        g_object_unref(pThis->cancellable);
        pThis->cancellable = nullptr;
    }

    char** types = g_mount_guess_content_type_finish(mount, res, nullptr);
    char* desc = nullptr;

    if(types) {
        if(types[0]) {
            for(char** type = types; *type; ++type) {
                GList* l = g_app_info_get_all_for_type(*type);
                if(l) {
                    pThis->applications = g_list_concat(pThis->applications, l);
                }
            }
            desc = g_content_type_get_description(types[0]);
        }
        g_strfreev(types);

        if(pThis->applications) {
            int pos = 0;
            for(GList* l = pThis->applications; l; l = l->next, ++pos) {
                GAppInfo* app = G_APP_INFO(l->data);
                GIcon* gicon = g_app_info_get_icon(app);
                QIcon icon = Fm::IconInfo::fromGIcon(gicon)->qicon();
                QString text = QString::fromUtf8(g_app_info_get_name(app));
                QListWidgetItem* item = new QListWidgetItem(icon, text);
                item->setData(Qt::UserRole, qVariantFromValue<void*>(app));
                pThis->ui.listWidget->insertItem(pos, item);
            }
        }
    }

    if(desc) {
        pThis->ui.mediumType->setText(QString::fromUtf8(desc));
        g_free(desc);
    }
    else {
        pThis->ui.mediumType->setText(tr("Removable Disk"));
    }

    // select the first item
    pThis->ui.listWidget->item(0)->setSelected(true);
}

} // namespace PCManFM
