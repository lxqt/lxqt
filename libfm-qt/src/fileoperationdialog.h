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


#ifndef FM_FILEOPERATIONDIALOG_H
#define FM_FILEOPERATIONDIALOG_H

#include "libfmqtglobals.h"
#include <cstdint>
#include <QDialog>
#include "core/filepath.h"
#include "core/fileinfo.h"
#include "core/fileoperationjob.h"

namespace Ui {
class FileOperationDialog;
}

namespace Fm {

class FileOperation;

class LIBFM_QT_API FileOperationDialog : public QDialog {
    Q_OBJECT
public:
    explicit FileOperationDialog(FileOperation* _operation);
    ~FileOperationDialog() override;

    void setSourceFiles(const Fm::FilePathList& srcFiles);
    void setDestPath(const Fm::FilePath& dest);

    int ask(QString question, char* const* options);

    FileOperationJob::FileExistsAction askRename(const FileInfo& src, const FileInfo& dest, FilePath& newDest);

    Job::ErrorAction error(GError* err, Job::ErrorSeverity severity);
    void setPrepared();
    void setCurFile(QString cur_file);
    void setPercent(unsigned int percent);
    void setDataTransferred(std::uint64_t finishedSize, std::uint64_t totalSize);
    void setFilesProcessed(std::uint64_t finishedCount, std::uint64_t totalCount);
    void setRemainingTime(unsigned int sec);

    void reject() override;

private:
    Ui::FileOperationDialog* ui;
    FileOperation* operation;
    int defaultOption;
    bool ignoreNonCriticalErrors_;
};

}

#endif // FM_FILEOPERATIONDIALOG_H
