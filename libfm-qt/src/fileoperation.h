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


#ifndef FM_FILEOPERATION_H
#define FM_FILEOPERATION_H

#include "libfmqtglobals.h"
#include <QObject>
#include <QElapsedTimer>
#include "core/filepath.h"
#include "core/fileoperationjob.h"

class QTimer;

namespace Fm {

class FileOperationDialog;

class LIBFM_QT_API FileOperation : public QObject {
    Q_OBJECT
public:
    enum Type {
        Copy,
        Move,
        Link,
        Delete,
        Trash,
        UnTrash,
        ChangeAttr
    };

public:
    explicit FileOperation(Type type, Fm::FilePathList srcFiles, QObject* parent = nullptr);

    ~FileOperation() override;

    void setDestination(Fm::FilePath dest);

    void setDestFiles(FilePathList destFiles);

    void setChmod(mode_t newMode, mode_t newModeMask);

    void setChown(uid_t uid, gid_t gid);

    // This only work for change attr jobs.
    void setRecursiveChattr(bool recursive);

    bool run();

    void cancel();

    bool isRunning() const {
        return job_ && !isCancelled();
    }

    bool isCancelled() const {
        if(job_) {
            return job_->isCancelled();
        }
        return false;
    }

    Fm::FileOperationJob* job() {
        return job_;
    }

    bool autoDestroy() {
        return autoDestroy_;
    }
    void setAutoDestroy(bool destroy = true) {
        autoDestroy_ = destroy;
    }

    Type type() {
        return type_;
    }

    // convinient static functions
    static FileOperation* copyFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent = nullptr);

    static FileOperation* copyFiles(Fm::FilePathList srcFiles, Fm::FilePathList destFiles, QWidget* parent = nullptr);

    static FileOperation* copyFile(Fm::FilePath srcFile, Fm::FilePath destFile, QWidget* parent = nullptr) {
        return copyFiles(FilePathList{std::move(srcFile)}, FilePathList{std::move(destFile)}, parent);
    }

    static FileOperation* moveFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent = nullptr);

    static FileOperation* moveFiles(Fm::FilePathList srcFiles, Fm::FilePathList destFiles, QWidget* parent = nullptr);

    static FileOperation* moveFile(Fm::FilePath srcFile, Fm::FilePath destFile, QWidget* parent = nullptr) {
        return moveFiles(FilePathList{std::move(srcFile)}, FilePathList{std::move(destFile)}, parent);
    }

    static FileOperation* symlinkFiles(Fm::FilePathList srcFiles, Fm::FilePath dest, QWidget* parent = nullptr);

    static FileOperation* symlinkFiles(Fm::FilePathList srcFiles, Fm::FilePathList destFiles, QWidget* parent = nullptr);

    static FileOperation* symlinkFile(Fm::FilePath srcFile, Fm::FilePath destFile, QWidget* parent = nullptr) {
        return symlinkFiles(FilePathList{std::move(srcFile)}, FilePathList{std::move(destFile)}, parent);
    }

    static FileOperation* deleteFiles(Fm::FilePathList srcFiles, bool promp = true, QWidget* parent = nullptr);

    static FileOperation* trashFiles(Fm::FilePathList srcFiles, bool promp = true, QWidget* parent = nullptr);

    static FileOperation* unTrashFiles(Fm::FilePathList srcFiles, QWidget* parent = nullptr);

    static FileOperation* changeAttrFiles(Fm::FilePathList srcFiles, QWidget* parent = nullptr);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void onJobPrepared();
    void onJobFinish();
    void onJobCancalled();
    void onJobError(const GErrorPtr& err, Fm::Job::ErrorSeverity severity, Fm::Job::ErrorAction& response);
    void onJobFileExists(const FileInfo& src, const FileInfo& dest, Fm::FileOperationJob::FileExistsAction& response, FilePath& newDest);

private:

    void disconnectJob();
    void showDialog();

    void pauseElapsedTimer() {
        if(Q_LIKELY(elapsedTimer_ != nullptr)) {
            lastElapsed_ += elapsedTimer_->elapsed();
            elapsedTimer_->invalidate();
        }
    }

    void resumeElapsedTimer() {
        if(Q_LIKELY(elapsedTimer_ != nullptr)) {
            elapsedTimer_->start();
        }
    }

    qint64 elapsedTime() {
        if(Q_LIKELY(elapsedTimer_ != nullptr)) {
            return lastElapsed_ + elapsedTimer_->elapsed();
        }
        return 0;
    }

private Q_SLOTS:
    void onUiTimeout();

private:
    Type type_;
    FileOperationJob* job_;
    FileOperationDialog* dlg_;
    FilePath destPath_;
    FilePath curFilePath_;
    FilePathList srcPaths_;
    QTimer* uiTimer_;
    QElapsedTimer* elapsedTimer_;
    qint64 lastElapsed_;
    bool updateRemainingTime_;
    QString curFile;
    bool autoDestroy_;
};

}

#endif // FM_FILEOPERATION_H
