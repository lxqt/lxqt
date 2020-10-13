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


#ifndef FM_FILELAUNCHER_H
#define FM_FILELAUNCHER_H

#include "libfmqtglobals.h"
#include <QWidget>
#include "core/fileinfo.h"
#include "core/basicfilelauncher.h"

namespace Fm {

class LIBFM_QT_API FileLauncher: public BasicFileLauncher {
public:
    explicit FileLauncher();
    ~FileLauncher() override;

    bool launchFiles(QWidget* parent, const FileInfoList& file_infos);

    bool launchPaths(QWidget* parent, const FilePathList &paths);

protected:

    GAppInfoPtr chooseApp(const FileInfoList& fileInfos, const char* mimeType, GErrorPtr& err) override;

    bool openFolder(GAppLaunchContext* ctx, const FileInfoList& folderInfos, GErrorPtr& err) override;

    bool showError(GAppLaunchContext* ctx, const GErrorPtr &err, const FilePath& path = FilePath{}, const FileInfoPtr& info = FileInfoPtr{}) override;

    ExecAction askExecFile(const FileInfoPtr& file) override;

    int ask(const char* msg, char* const* btn_labels, int default_btn) override;
};

}

#endif // FM_FILELAUNCHER_H
