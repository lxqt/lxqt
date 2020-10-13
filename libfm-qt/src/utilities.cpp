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

#include "utilities.h"
#include "utilities_p.h"
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QUrl>
#include <QList>
#include <QStringBuilder>
#include <QMessageBox>
#include <QStandardPaths>
#include "fileoperation.h"
#include <QEventLoop>

#include <pwd.h>
#include <grp.h>
#include <cstdlib>
#include <glib.h>

namespace Fm {

Fm::FilePathList pathListFromUriList(const char* uriList) {
    Fm::FilePathList pathList;
    char** uris = g_strsplit_set(uriList, "\r\n", -1);
    for(char** uri = uris; *uri; ++uri) {
        if(**uri != '\0') {
            pathList.push_back(Fm::FilePath::fromUri(*uri));
        }
    }
    g_strfreev(uris);
    return pathList;
}

QByteArray pathListToUriList(const Fm::FilePathList& paths) {
    QByteArray uriList;
    for(auto& path: paths) {
        uriList += path.uri().get();
        uriList += "\r\n";
    }
    return uriList;
}

Fm::FilePathList pathListFromQUrls(QList<QUrl> urls) {
    Fm::FilePathList pathList;
    for(auto it = urls.cbegin(); it != urls.cend(); ++it) {
        auto path = Fm::FilePath::fromUri(it->toString().toUtf8().constData());
        pathList.push_back(std::move(path));
    }
    return pathList;
}

void pasteFilesFromClipboard(const Fm::FilePath& destPath, QWidget* parent) {
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    Fm::FilePathList paths;
    bool isCut = false;

    if(data->hasFormat(QStringLiteral("x-special/gnome-copied-files"))) {
        // Gnome, LXDE, and XFCE
        QByteArray gnomeData = data->data(QStringLiteral("x-special/gnome-copied-files"));
        char* pdata = gnomeData.data();
        char* eol = strchr(pdata, '\n');

        if(eol) {
            *eol = '\0';
            isCut = (strcmp(pdata, "cut") == 0 ? true : false);
            paths = pathListFromUriList(eol + 1);
        }
    }

    if(paths.empty() && data->hasUrls()) {
        // The KDE way
        paths = Fm::pathListFromQUrls(data->urls());
        QByteArray cut = data->data(QStringLiteral("application/x-kde-cutselection"));
        if(!cut.isEmpty() && QChar::fromLatin1(cut.at(0)) == QLatin1Char('1')) {
            isCut = true;
        }
    }

    if(!paths.empty()) {
        if(isCut) {
            FileOperation::moveFiles(paths, destPath, parent);
            clipboard->clear(QClipboard::Clipboard);
        }
        else {
            FileOperation::copyFiles(paths, destPath, parent);
        }
    }
}

void copyFilesToClipboard(const Fm::FilePathList& files) {
    QClipboard* clipboard = QApplication::clipboard();
    QMimeData* data = new QMimeData();
    auto urilist = pathListToUriList(files);

    // Gnome, LXDE, and XFCE
    // Note: the standard text/urilist format uses CRLF for line breaks, but gnome format uses LF only
    data->setData(QStringLiteral("x-special/gnome-copied-files"), QByteArray("copy\n") + urilist.replace("\r\n", "\n"));
    // The KDE way
    data->setData(QStringLiteral("text/uri-list"), urilist);
    // data->setData(QStringLiteral("application/x-kde-cutselection"), QByteArrayLiteral("0"));
    clipboard->setMimeData(data);
}

void cutFilesToClipboard(const Fm::FilePathList& files) {
    QClipboard* clipboard = QApplication::clipboard();
    QMimeData* data = new QMimeData();
    auto urilist = pathListToUriList(files);

    // Gnome, LXDE, and XFCE
    // Note: the standard text/urilist format uses CRLF for line breaks, but gnome format uses LF only
    data->setData(QStringLiteral("x-special/gnome-copied-files"), QByteArray("cut\n") + urilist.replace("\r\n", "\n"));
    // The KDE way
    data->setData(QStringLiteral("text/uri-list"), urilist);
    data->setData(QStringLiteral("application/x-kde-cutselection"), QByteArrayLiteral("1"));
    clipboard->setMimeData(data);
}

bool changeFileName(const Fm::FilePath& filePath, const QString& newName, QWidget* parent, bool showMessage) {
    // NOTE: g_file_set_display_name() is used instead of g_file_move() because,
    // otherwise, renaming will not be possible in places like google-drive:///.
    Fm::GErrorPtr err;
    GFilePtr gfile{g_file_set_display_name(filePath.gfile().get(),
                                            newName.toLocal8Bit().constData(),
                                            nullptr, /* make this cancellable later. */
                                            &err)};
    if(gfile == nullptr) {
        if (showMessage){
            QMessageBox::critical(parent ? parent->window() : nullptr, QObject::tr("Error"), err.message());
        }
        return false;
    }

    // reload the containing folder if it is in use but does not have a file monitor
    auto folder = Fm::Folder::findByPath(filePath.parent());
    if(folder && folder->isValid() && folder->isLoaded() && !folder->hasFileMonitor()) {
        folder->reload();
    }

    return true;
}

bool renameFile(std::shared_ptr<const Fm::FileInfo> file, QWidget* parent) {
    FilenameDialog dlg(parent ? parent->window() : nullptr);
    dlg.setWindowTitle(QObject::tr("Rename File"));
    dlg.setLabelText(QObject::tr("Please enter a new name:"));
    // NOTE: "Edit name" seems the best way to handle non-UTF8 filename encoding.
    auto old_name = QString::fromUtf8(g_file_info_get_edit_name(file->gFileInfo().get()));
    if(old_name.isEmpty()) {
        old_name = QString::fromStdString(file->name());
    }
    dlg.setTextValue(old_name);

    if(file->isDir()) { // select filename extension for directories
        dlg.setSelectExtension(true);
    }

    if(dlg.exec() != QDialog::Accepted) {
        return false; // stop multiple renaming
    }

    QString new_name = dlg.textValue();
    if(new_name == old_name) {
        return true; // let multiple renaming continue
    }
    changeFileName(file->path(), new_name, parent);
    return true;
}

void setDefaultAppForType(const Fm::GAppInfoPtr app, std::shared_ptr<const Fm::MimeType> mimeType) {
    // NOTE: "g_app_info_set_as_default_for_type()" writes to "~/.config/mimeapps.list"
    // but we want to set the default app only for the current DE (e.g., LXQt).
    // More importantly, if the DE-specific list already exists and contains some
    // default apps, it will have priority over "~/.config/mimeapps.list" and so,
    // "g_app_info_set_as_default_for_type()" could not change those apps.

    if(app == nullptr || mimeType == nullptr) {
        return;
    }

    // first find the DE's mimeapps list file
    QByteArray mimeappsList = "mimeapps.list";
    QList<QByteArray> desktopsList = qgetenv("XDG_CURRENT_DESKTOP").toLower().split(':');
    if(!desktopsList.isEmpty()) {
        mimeappsList = desktopsList.at(0) + "-" + mimeappsList;
    }
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    auto mimeappsListPath = CStrPtr(g_build_filename(configDir.toUtf8().constData(),
                                                     mimeappsList.constData(),
                                                     nullptr));

    // set the default app in the DE's mimeapps list
    const char* desktop_id = g_app_info_get_id(app.get());
    GKeyFile* kf = g_key_file_new();
    g_key_file_load_from_file(kf, mimeappsListPath.get(), G_KEY_FILE_NONE, nullptr);
    g_key_file_set_string(kf, "Default Applications", mimeType->name(), desktop_id);
    g_key_file_save_to_file(kf, mimeappsListPath.get(), nullptr);
    g_key_file_free(kf);
}

// templateFile is a file path used as a template of the new file.
void createFileOrFolder(CreateFileType type, FilePath parentDir, const TemplateItem* templ, QWidget* parent) {
    QString defaultNewName;
    QString prompt;
    QString dialogTitle = type == CreateNewFolder ? QObject::tr("Create Folder")
                          : QObject::tr("Create File");

    switch(type) {
    case CreateNewTextFile:
        prompt = QObject::tr("Please enter a new file name:");
        defaultNewName = QObject::tr("New text file");
        break;

    case CreateNewFolder:
        prompt = QObject::tr("Please enter a new folder name:");
        defaultNewName = QObject::tr("New folder");
        break;

    case CreateWithTemplate: {
        auto mime = templ->mimeType();
        prompt = QObject::tr("Enter a name for the new %1:").arg(QString::fromUtf8(mime->desc()));
        defaultNewName = QString::fromStdString(templ->name());
    }
    break;
    }

_retry:
    // ask the user to input a file name
    bool ok;
    QString new_name = QInputDialog::getText(parent ? parent->window() : nullptr,
                       dialogTitle,
                       prompt,
                       QLineEdit::Normal,
                       defaultNewName,
                       &ok);

    if(!ok) {
        return;
    }

    auto dest = parentDir.child(new_name.toLocal8Bit().data());
    Fm::GErrorPtr err;
    switch(type) {
    case CreateNewTextFile: {
        Fm::GFileOutputStreamPtr f{g_file_create(dest.gfile().get(), G_FILE_CREATE_NONE, nullptr, &err), false};
        if(f) {
            g_output_stream_close(G_OUTPUT_STREAM(f.get()), nullptr, nullptr);
        }
        break;
    }
    case CreateNewFolder:
        g_file_make_directory(dest.gfile().get(), nullptr, &err);
        break;
    case CreateWithTemplate:
        // copy the template file to its destination
        FileOperation::copyFile(templ->filePath(), dest, parent);
        break;
    }
    if(err) {
        if(err.domain() == G_IO_ERROR && err.code() == G_IO_ERROR_EXISTS) {
            err.reset();
            goto _retry;
        }

        QMessageBox::critical(parent ? parent->window() : nullptr, QObject::tr("Error"), err.message());
    }
    else { // reload the containing folder if it is in use but does not have a file monitor
        auto folder = Fm::Folder::findByPath(parentDir);
        if(folder && folder->isValid() && folder->isLoaded() && !folder->hasFileMonitor()) {
            folder->reload();
        }
    }
}

uid_t uidFromName(QString name) {
    uid_t ret;
    if(name.isEmpty()) {
        return INVALID_UID;
    }
    if(name.at(0).digitValue() != -1) {
        ret = uid_t(name.toUInt());
    }
    else {
        struct passwd* pw = getpwnam(name.toLatin1().constData());
        // FIXME: use getpwnam_r instead later to make it reentrant
        ret = pw ? pw->pw_uid : INVALID_UID;
    }

    return ret;
}

QString uidToName(uid_t uid) {
    QString ret;
    struct passwd* pw = getpwuid(uid);

    if(pw) {
        ret = QString::fromUtf8(pw->pw_name);
    }
    else {
        ret = QString::number(uid);
    }

    return ret;
}

gid_t gidFromName(QString name) {
    gid_t ret;
    if(name.isEmpty()) {
        return INVALID_GID;
    }
    if(name.at(0).digitValue() != -1) {
        ret = gid_t(name.toUInt());
    }
    else {
        // FIXME: use getgrnam_r instead later to make it reentrant
        struct group* grp = getgrnam(name.toLatin1().constData());
        ret = grp ? grp->gr_gid : INVALID_GID;
    }

    return ret;
}

QString gidToName(gid_t gid) {
    QString ret;
    struct group* grp = getgrgid(gid);

    if(grp) {
        ret = QString::fromUtf8(grp->gr_name);
    }
    else {
        ret = QString::number(gid);
    }

    return ret;
}

int execModelessDialog(QDialog* dlg) {
    // FIXME: this does much less than QDialog::exec(). Will this work flawlessly?
    QEventLoop loop;
    QObject::connect(dlg, &QDialog::finished, &loop, &QEventLoop::quit);
    // DialogExec does not seem to be documented in the Qt API doc?
    // However, in the source code of QDialog::exec(), it's used so let's use it too.
    dlg->show();
    (void)loop.exec(QEventLoop::DialogExec);
    return dlg->result();
}

// check if GVFS can support this uri scheme (lower case)
// NOTE: this does not work reliably due to some problems in gio/gvfs and causes bug lxqt/lxqt#512
// https://github.com/lxqt/lxqt/issues/512
// Use uriExists() whenever possible.
bool isUriSchemeSupported(const char* uriScheme) {
    const gchar* const* schemes = g_vfs_get_supported_uri_schemes(g_vfs_get_default());
    if(Q_UNLIKELY(schemes == nullptr)) {
        return false;
    }
    for(const gchar * const* scheme = schemes; *scheme; ++scheme)
        if(strcmp(uriScheme, *scheme) == 0) {
            return true;
        }
    return false;
}

// check if the URI exists.
// NOTE: this is a blocking call possibly involving I/O.
// So it's better to use it in limited cases, like checking trash:// or computer://.
// Avoid calling this on a slow filesystem.
// Checking "network:///" is very slow, for example.
bool uriExists(const char* uri) {
    GFile* gf = g_file_new_for_uri(uri);
    bool ret = (bool)g_file_query_exists(gf, nullptr);
    g_object_unref(gf);
    return ret;
}

QString formatFileSize(uint64_t size, bool useSI) {
    Fm::CStrPtr str{g_format_size_full(size, useSI ? G_FORMAT_SIZE_DEFAULT : G_FORMAT_SIZE_IEC_UNITS)};
    return QString(QString::fromUtf8(str.get()));
}

} // namespace Fm
