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

#ifndef FM_UTILITIES_H
#define FM_UTILITIES_H

#include "libfmqtglobals.h"
#include <QUrl>
#include <QList>
#include <QMimeData>
#include <sys/types.h>

#include <cstdint>
#include <utility>

#include "core/filepath.h"
#include "core/fileinfo.h"
#include "core/templates.h"

class QDialog;

namespace Fm {

LIBFM_QT_API Fm::FilePathList pathListFromUriList(const char* uriList);

LIBFM_QT_API QByteArray pathListToUriList(const Fm::FilePathList& paths);

LIBFM_QT_API Fm::FilePathList pathListFromQUrls(QList<QUrl> urls);

LIBFM_QT_API void pasteFilesFromClipboard(const Fm::FilePath& destPath, QWidget* parent = nullptr);

LIBFM_QT_API void copyFilesToClipboard(const Fm::FilePathList& files);

LIBFM_QT_API void cutFilesToClipboard(const Fm::FilePathList& files);

LIBFM_QT_API bool changeFileName(const Fm::FilePath& path, const QString& newName, QWidget* parent, bool showMessage = true);

LIBFM_QT_API bool renameFile(std::shared_ptr<const Fm::FileInfo> file, QWidget* parent = nullptr);

LIBFM_QT_API void setDefaultAppForType(const Fm::GAppInfoPtr app, std::shared_ptr<const Fm::MimeType> mimeType);

enum CreateFileType {
    CreateNewFolder,
    CreateNewTextFile,
    CreateWithTemplate
};

LIBFM_QT_API void createFileOrFolder(CreateFileType type, FilePath parentDir, const TemplateItem* templ = nullptr, QWidget* parent = nullptr);

constexpr uid_t INVALID_UID = uid_t(-1);

LIBFM_QT_API uid_t uidFromName(QString name);

LIBFM_QT_API QString uidToName(uid_t uid);

constexpr gid_t INVALID_GID = gid_t(-1);

LIBFM_QT_API gid_t gidFromName(QString name);

LIBFM_QT_API QString gidToName(gid_t gid);

LIBFM_QT_API int execModelessDialog(QDialog* dlg);

// NOTE: this does not work reliably due to some problems in gio/gvfs
// Use uriExists() whenever possible.
LIBFM_QT_API bool isUriSchemeSupported(const char* uriScheme);

LIBFM_QT_API bool uriExists(const char* uri);

LIBFM_QT_API QString formatFileSize(std::uint64_t size, bool useSI = false);

}

#endif // FM_UTILITIES_H
