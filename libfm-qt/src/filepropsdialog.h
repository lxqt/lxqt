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


#ifndef FM_FILEPROPSDIALOG_H
#define FM_FILEPROPSDIALOG_H

#include "libfmqtglobals.h"
#include <QDialog>
#include <QTimer>

#include "core/fileinfo.h"
#include "core/totalsizejob.h"

namespace Ui {
class FilePropsDialog;
}

namespace Fm {

class LIBFM_QT_API FilePropsDialog : public QDialog {
    Q_OBJECT

public:
    explicit FilePropsDialog(Fm::FileInfoList files, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~FilePropsDialog() override;

    void accept() override;

    static FilePropsDialog* showForFile(std::shared_ptr<const Fm::FileInfo> file, QWidget* parent = nullptr) {
        Fm::FileInfoList files;
        files.push_back(std::move(file));
        FilePropsDialog* dlg = showForFiles(files, parent);
        return dlg;
    }

    static FilePropsDialog* showForFiles(Fm::FileInfoList files, QWidget* parent = nullptr) {
        FilePropsDialog* dlg = new FilePropsDialog(std::move(files), parent);
        dlg->show();
        return dlg;
    }

private:
    void initGeneralPage();
    void initApplications();
    void initPermissionsPage();
    void initOwner();

private Q_SLOTS:
    void onDeepCountJobFinished();
    void onFileSizeTimerTimeout();
    void onIconButtonclicked();

private:
    Ui::FilePropsDialog* ui;
    Fm::FileInfoList fileInfos_; // list of all file infos
    std::shared_ptr<const Fm::FileInfo> fileInfo; // file info of the first file in the list
    bool singleType; // all files are of the same type?
    bool singleFile; // only one file is selected?
    bool hasDir; // is there any dir in the files?
    bool allNative; // all files are on native UNIX filesystems (not virtual or remote)
    QIcon customIcon; // custom (folder) icon (wiil be checked for its nullity)

    std::shared_ptr<const Fm::MimeType> mimeType; // mime type of the files

    uid_t uid; // owner uid of the files, INVALID_UID means all files do not have the same uid
    gid_t gid; // owner gid of the files, INVALID_GID means all files do not have the same uid
    mode_t ownerPerm; // read permission of the files, -1 means not all files have the same value
    int ownerPermSel;
    mode_t groupPerm; // read permission of the files, -1 means not all files have the same value
    int groupPermSel;
    mode_t otherPerm; // read permission of the files, -1 means not all files have the same value
    int otherPermSel;
    mode_t execPerm; // exec permission of the files
    Qt::CheckState execCheckState;

    Fm::TotalSizeJob* totalSizeJob; // job used to count total size
    QTimer* fileSizeTimer;
};

}

#endif // FM_FILEPROPSDIALOG_H
