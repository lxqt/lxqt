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

#include "appchooserdialog.h"
#include "ui_app-chooser-dialog.h"
#include "utilities.h"
#include <QPushButton>
#include <gio/gdesktopappinfo.h>
#include <glib/gstdio.h>

namespace Fm {

AppChooserDialog::AppChooserDialog(std::shared_ptr<const Fm::MimeType> mimeType, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    ui(new Ui::AppChooserDialog()),
    mimeType_{std::move(mimeType)},
    canSetDefault_(true) {
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(ui->appMenuView, &AppMenuView::selectionChanged, this, &AppChooserDialog::onSelectionChanged);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &AppChooserDialog::onTabChanged);
    if(!ui->appMenuView->isAppSelected()) {
        ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(false);    // disable OK button
    }
}

AppChooserDialog::~AppChooserDialog() {
    delete ui;
}

bool AppChooserDialog::isSetDefault() const {
    return ui->setDefault->isChecked();
}

static void on_temp_appinfo_destroy(gpointer data, GObject* /*objptr*/) {
    char* filename = (char*)data;
    if(g_unlink(filename) < 0) {
        g_critical("failed to remove %s", filename);
    }
    /* else
        qDebug("temp file %s removed", filename); */
    g_free(filename);
}

static GAppInfo* app_info_create_from_commandline(const char* commandline,
        const char* application_name,
        const char* bin_name,
        const char* mime_type,
        gboolean terminal, gboolean keep) {
    GAppInfo* app = nullptr;
    char* dirname = g_build_filename(g_get_user_data_dir(), "applications", nullptr);
    const char* app_basename = strrchr(bin_name, '/');

    if(app_basename) {
        app_basename++;
    }
    else {
        app_basename = bin_name;
    }
    if(g_mkdir_with_parents(dirname, 0700) == 0) {
        char* filename = g_strdup_printf("%s/userapp-%s-XXXXXX.desktop", dirname, app_basename);
        int fd = g_mkstemp(filename);
        if(fd != -1) {
            GString* content = g_string_sized_new(256);
            g_string_printf(content,
                            "[" G_KEY_FILE_DESKTOP_GROUP "]\n"
                            G_KEY_FILE_DESKTOP_KEY_TYPE "=" G_KEY_FILE_DESKTOP_TYPE_APPLICATION "\n"
                            G_KEY_FILE_DESKTOP_KEY_NAME "=%s\n"
                            G_KEY_FILE_DESKTOP_KEY_EXEC "=%s\n"
                            G_KEY_FILE_DESKTOP_KEY_CATEGORIES "=Other;\n"
                            G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY "=true\n",
                            application_name,
                            commandline
                           );
            if(mime_type)
                g_string_append_printf(content,
                                       G_KEY_FILE_DESKTOP_KEY_MIME_TYPE "=%s\n",
                                       mime_type);
            g_string_append_printf(content,
                                   G_KEY_FILE_DESKTOP_KEY_TERMINAL "=%s\n",
                                   terminal ? "true" : "false");
            if(terminal)
                g_string_append_printf(content, "X-KeepTerminal=%s\n",
                                       keep ? "true" : "false");
            close(fd); /* g_file_set_contents() may fail creating duplicate */
            if(g_file_set_contents(filename, content->str, content->len, nullptr)) {
                char* fbname = g_path_get_basename(filename);
                app = G_APP_INFO(g_desktop_app_info_new(fbname));
                g_free(fbname);
                /* if there is mime_type set then created application will be
                   saved for the mime type (see fm_choose_app_for_mime_type()
                   below) but if not then we should remove this temp. file */
                if(!mime_type || !application_name[0])
                    /* save the name so this file will be removed later */
                    g_object_weak_ref(G_OBJECT(app), on_temp_appinfo_destroy,
                                      g_strdup(filename));
            }
            else {
                g_unlink(filename);
            }
            g_string_free(content, TRUE);
        }
        g_free(filename);
    }
    g_free(dirname);
    return app;
}

inline static char* get_binary(const char* cmdline, gboolean* arg_found) {
    /* see if command line contains %f, %F, %u, or %U. */
    const char* p = strstr(cmdline, " %");
    if(p) {
        if(!strchr("fFuU", *(p + 2))) {
            p = nullptr;
        }
    }
    if(arg_found) {
        *arg_found = (p != nullptr);
    }
    if(p) {
        return g_strndup(cmdline, p - cmdline);
    }
    else {
        return g_strdup(cmdline);
    }
}

GAppInfo* AppChooserDialog::customCommandToApp() {
    GAppInfo* app = nullptr;
    QByteArray cmdline = ui->cmdLine->text().toLocal8Bit();
    QByteArray app_name = ui->appName->text().toUtf8();
    if(!cmdline.isEmpty()) {
        gboolean arg_found = FALSE;
        char* bin1 = get_binary(cmdline.constData(), &arg_found);
        qDebug("bin1 = %s", bin1);
        /* see if command line contains %f, %F, %u, or %U. */
        if(!arg_found) {  /* append %f if no %f, %F, %u, or %U was found. */
            cmdline += " %f";
        }

        /* FIXME: is there any better way to do this? */
        /* We need to ensure that no duplicated items are added */
        if(mimeType_) {
            MenuCache* menu_cache;
            /* see if the command is already in the list of known apps for this mime-type */
            GList* apps = g_app_info_get_all_for_type(mimeType_->name());
            GList* l;
            for(l = apps; l; l = l->next) {
                GAppInfo* app2 = G_APP_INFO(l->data);
                const char* cmd = g_app_info_get_commandline(app2);
                char* bin2 = get_binary(cmd, nullptr);
                if(g_strcmp0(bin1, bin2) == 0) {
                    app = G_APP_INFO(g_object_ref(app2));
                    qDebug("found in app list");
                    g_free(bin2);
                    break;
                }
                g_free(bin2);
            }
            g_list_free_full(apps, g_object_unref);
            if(app) {
                goto _out;
            }

            /* see if this command can be found in menu cache */
            menu_cache = menu_cache_lookup("applications.menu");
            if(menu_cache) {
                MenuCacheDir* root_dir = menu_cache_dup_root_dir(menu_cache);
                if(root_dir) {
                    GSList* all_apps = menu_cache_list_all_apps(menu_cache);
                    GSList* l;
                    for(l = all_apps; l; l = l->next) {
                        MenuCacheApp* ma = MENU_CACHE_APP(l->data);
                        const char* exec = menu_cache_app_get_exec(ma);
                        char* bin2;
                        if(exec == nullptr) {
                            g_warning("application %s has no Exec statement", menu_cache_item_get_id(MENU_CACHE_ITEM(ma)));
                            continue;
                        }
                        bin2 = get_binary(exec, nullptr);
                        if(g_strcmp0(bin1, bin2) == 0) {
                            app = G_APP_INFO(g_desktop_app_info_new(menu_cache_item_get_id(MENU_CACHE_ITEM(ma))));
                            qDebug("found in menu cache");
                            menu_cache_item_unref(MENU_CACHE_ITEM(ma));
                            g_free(bin2);
                            break;
                        }
                        menu_cache_item_unref(MENU_CACHE_ITEM(ma));
                        g_free(bin2);
                    }
                    g_slist_free(all_apps);
                    menu_cache_item_unref(MENU_CACHE_ITEM(root_dir));
                }
                menu_cache_unref(menu_cache);
            }
            if(app) {
                goto _out;
            }
        }

        /* FIXME: g_app_info_create_from_commandline force the use of %f or %u, so this is not we need */
        app = app_info_create_from_commandline(cmdline.constData(), app_name.constData(), bin1,
                                               mimeType_ ? mimeType_->name() : nullptr,
                                               ui->useTerminal->isChecked(), ui->keepTermOpen->isChecked());
_out:
        g_free(bin1);
    }
    return app;
}

void AppChooserDialog::accept() {
    QDialog::accept();

    if(ui->tabWidget->currentIndex() == 0) {
        selectedApp_ = ui->appMenuView->selectedApp();
    }
    else { // custom command line
        selectedApp_ = customCommandToApp();
    }

    if(selectedApp_) {
        if(mimeType_ && g_app_info_get_name(selectedApp_.get())) {
            /* add this app to the mime-type */
#if GLIB_CHECK_VERSION(2, 27, 6)
            g_app_info_set_as_last_used_for_type(selectedApp_.get(), mimeType_->name(), nullptr);
#else
            g_app_info_add_supports_type(selectedApp_.get(), mimeType_->name(), nullptr);
#endif
            /* if need to set default */
            if(ui->setDefault->isChecked()) {
                setDefaultAppForType(selectedApp_, mimeType_);
            }
        }
    }
}

void AppChooserDialog::onSelectionChanged() {
    bool isAppSelected = ui->appMenuView->isAppSelected();
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(isAppSelected);
}

void AppChooserDialog::setMimeType(std::shared_ptr<const Fm::MimeType> mimeType) {
    mimeType_ = std::move(mimeType);
    if(mimeType_) {
        QString text = tr("Select an application to open \"%1\" files")
                       .arg(QString::fromUtf8(mimeType_->desc()));
        ui->fileTypeHeader->setText(text);
    }
    else {
        ui->fileTypeHeader->hide();
        ui->setDefault->hide();
    }
}

void AppChooserDialog::setCanSetDefault(bool value) {
    canSetDefault_ = value;
    ui->setDefault->setVisible(value);
}

void AppChooserDialog::onTabChanged(int index) {
    if(index == 0) { // app menu view
        onSelectionChanged();
    }
    else if(index == 1) { // custom command
        ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

} // namespace Fm
