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


#include "filepropsdialog.h"
#include "ui_file-props.h"
#include "utilities.h"
#include "fileoperation.h"
#include <QStringBuilder>
#include <QStringListModel>
#include <QMessageBox>
#include <QDateTime>
#include <QStandardPaths>
#include <QFileDialog>
#include <sys/types.h>
#include <ctime>
#include "core/totalsizejob.h"
#include "core/folder.h"
#include <QPushButton>
#include "core/legacy/fm-config.h"

#define DIFFERENT_UIDS    ((uid)-1)
#define DIFFERENT_GIDS    ((gid)-1)
#define DIFFERENT_PERMS   ((mode_t)-1)

namespace Fm {

enum {
    ACCESS_NO_CHANGE = 0,
    ACCESS_READ_ONLY,
    ACCESS_READ_WRITE,
    ACCESS_FORBID
};

FilePropsDialog::FilePropsDialog(Fm::FileInfoList files, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    fileInfos_{std::move(files)},
    fileInfo{fileInfos_.front()},
    singleType(fileInfos_.isSameType()),
    singleFile(fileInfos_.size() == 1 ? true : false) {

    setAttribute(Qt::WA_DeleteOnClose);

    ui = new Ui::FilePropsDialog();
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    if(singleType) {
        mimeType = fileInfo->mimeType();
    }
    totalSizeJob = new Fm::TotalSizeJob(fileInfos_.paths(), Fm::TotalSizeJob::DEFAULT);

    initGeneralPage();
    initPermissionsPage();

    if(!singleFile || !hasDir) { // not a single dir
        ui->contentsLabel->hide();
        ui->fileNumber->hide();
    }
}

FilePropsDialog::~FilePropsDialog() {
    // Stop the timer if it's still running
    if(fileSizeTimer) {
        fileSizeTimer->stop();
        delete fileSizeTimer;
        fileSizeTimer = nullptr;
    }

    // Cancel the indexing job if it hasn't finished
    if(totalSizeJob) {
        totalSizeJob->cancel();
        totalSizeJob = nullptr;
    }

    // And finally delete the dialog's UI
    delete ui;
}

void FilePropsDialog::initApplications() {
    if(singleType && mimeType && !fileInfo->isDir()) {
        ui->openWith->setMimeType(mimeType);
    }
    else {
        ui->openWith->hide();
        ui->openWithLabel->hide();
    }
}

void FilePropsDialog::initPermissionsPage() {
    // ownership handling
    // get owner/group and mode of the first file in the list
    uid = fileInfo->uid();
    gid = fileInfo->gid();
    mode_t mode = fileInfo->mode();
    ownerPerm = (mode & (S_IRUSR | S_IWUSR | S_IXUSR));
    groupPerm = (mode & (S_IRGRP | S_IWGRP | S_IXGRP));
    otherPerm = (mode & (S_IROTH | S_IWOTH | S_IXOTH));
    execPerm = (mode & (S_IXUSR | S_IXGRP | S_IXOTH));
    allNative = fileInfo->isNative();
    hasDir = S_ISDIR(mode);

    // check if all selected files belongs to the same owner/group or have the same mode
    // at the same time, check if all files are on native unix filesystems
    for(auto& fi: fileInfos_) {
        if(allNative && !fi->isNative()) {
            allNative = false;    // not all of the files are native
        }

        mode_t fi_mode = fi->mode();
        if(S_ISDIR(fi_mode)) {
            hasDir = true;    // the files list contains dir(s)
        }

        if(uid != DIFFERENT_UIDS && static_cast<uid_t>(uid) != fi->uid()) {
            uid = DIFFERENT_UIDS;    // not all files have the same owner
        }
        if(gid != DIFFERENT_GIDS && static_cast<gid_t>(gid) != fi->gid()) {
            gid = DIFFERENT_GIDS;    // not all files have the same owner group
        }

        if(ownerPerm != DIFFERENT_PERMS && ownerPerm != (fi_mode & (S_IRUSR | S_IWUSR | S_IXUSR))) {
            ownerPerm = DIFFERENT_PERMS;    // not all files have the same permission for owner
        }
        if(groupPerm != DIFFERENT_PERMS && groupPerm != (fi_mode & (S_IRGRP | S_IWGRP | S_IXGRP))) {
            groupPerm = DIFFERENT_PERMS;    // not all files have the same permission for grop
        }
        if(otherPerm != DIFFERENT_PERMS && otherPerm != (fi_mode & (S_IROTH | S_IWOTH | S_IXOTH))) {
            otherPerm = DIFFERENT_PERMS;    // not all files have the same permission for other
        }
        if(execPerm != DIFFERENT_PERMS && execPerm != (fi_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
            execPerm = DIFFERENT_PERMS;    // not all files have the same executable permission
        }
    }

    // init owner/group
    initOwner();

    // if all files are of the same type, and some of them are dirs => all of the items are dirs
    // rwx values have different meanings for dirs
    // Let's make it clear for the users
    // init combo boxes for file permissions here
    QStringList comboItems;
    comboItems.append(QStringLiteral("---")); // no change
    if(singleType && hasDir) { // all files are dirs
        comboItems.append(tr("View folder content"));
        comboItems.append(tr("View and modify folder content"));
        ui->executable->hide();
    }
    else { //not all of the files are dirs
        comboItems.append(tr("Read"));
        comboItems.append(tr("Read and write"));
    }
    comboItems.append(tr("Forbidden"));
    QStringListModel* comboModel = new QStringListModel(comboItems, this);
    ui->ownerPerm->setModel(comboModel);
    ui->groupPerm->setModel(comboModel);
    ui->otherPerm->setModel(comboModel);

    // owner
    ownerPermSel = ACCESS_NO_CHANGE;
    if(ownerPerm != DIFFERENT_PERMS) { // permissions for owner are the same among all files
        if(ownerPerm & S_IRUSR) { // can read
            if(ownerPerm & S_IWUSR) { // can write
                ownerPermSel = ACCESS_READ_WRITE;
            }
            else {
                ownerPermSel = ACCESS_READ_ONLY;
            }
        }
        else {
            if((ownerPerm & S_IWUSR) == 0) { // cannot read or write
                ownerPermSel = ACCESS_FORBID;
            }
        }
    }
    ui->ownerPerm->setCurrentIndex(ownerPermSel);

    // owner and group
    groupPermSel = ACCESS_NO_CHANGE;
    if(groupPerm != DIFFERENT_PERMS) { // permissions for owner are the same among all files
        if(groupPerm & S_IRGRP) { // can read
            if(groupPerm & S_IWGRP) { // can write
                groupPermSel = ACCESS_READ_WRITE;
            }
            else {
                groupPermSel = ACCESS_READ_ONLY;
            }
        }
        else {
            if((groupPerm & S_IWGRP) == 0) { // cannot read or write
                groupPermSel = ACCESS_FORBID;
            }
        }
    }
    ui->groupPerm->setCurrentIndex(groupPermSel);

    // other
    otherPermSel = ACCESS_NO_CHANGE;
    if(otherPerm != DIFFERENT_PERMS) { // permissions for owner are the same among all files
        if(otherPerm & S_IROTH) { // can read
            if(otherPerm & S_IWOTH) { // can write
                otherPermSel = ACCESS_READ_WRITE;
            }
            else {
                otherPermSel = ACCESS_READ_ONLY;
            }
        }
        else {
            if((otherPerm & S_IWOTH) == 0) { // cannot read or write
                otherPermSel = ACCESS_FORBID;
            }
        }

    }
    ui->otherPerm->setCurrentIndex(otherPermSel);

    // set the checkbox to partially checked state
    // when owner, group, and other have different executable flags set.
    // some of them have exec, and others do not have.
    execCheckState = Qt::PartiallyChecked;
    if(execPerm != DIFFERENT_PERMS) { // if all files have the same executable permission
        // check if the files are all executable
        if((mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == (S_IXUSR | S_IXGRP | S_IXOTH)) {
            // owner, group, and other all have exec permission.
            ui->executable->setTristate(false);
            execCheckState = Qt::Checked;
        }
        else if((mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) {
            // owner, group, and other all have no exec permission
            ui->executable->setTristate(false);
            execCheckState = Qt::Unchecked;
        }
    }
    ui->executable->setCheckState(execCheckState);
}

void FilePropsDialog::initGeneralPage() {
    // update UI
    if(singleType) { // all files are of the same mime-type
        std::shared_ptr<const Fm::IconInfo> icon;
        // FIXME: handle custom icons for some files
        // FIXME: display special property pages for special files or
        // some specified mime-types.
        if(singleFile) { // only one file is selected.
            icon = fileInfo->icon();
        }
        if(mimeType) {
            if(!icon) { // get an icon from mime type if needed
                icon = mimeType->icon();
            }
            ui->fileType->setText(QString::fromUtf8(mimeType->desc()));
            ui->mimeType->setText(QString::fromUtf8(mimeType->name()));
        }
        if(icon) {
            ui->iconButton->setIcon(icon->qicon());
        }

        if(singleFile && fileInfo->isSymlink()) {
            ui->target->setText(QString::fromStdString(fileInfo->target()));
        }
        else {
            ui->target->hide();
            ui->targetLabel->hide();
        }
        if(fileInfo->isDir() && fileInfo->isNative()) { // all files are native dirs
            connect(ui->iconButton, &QAbstractButton::clicked, this, &FilePropsDialog::onIconButtonclicked);
        }
    } // end if(singleType)
    else { // not singleType, multiple files are selected at the same time
        ui->fileType->setText(tr("Files of different types"));
        ui->target->hide();
        ui->targetLabel->hide();
    }

    // FIXME: check if all files has the same parent dir, mtime, or atime
    if(singleFile) { // only one file is selected
        auto parent_path = fileInfo->path().parent();
        auto parent_str = parent_path ? parent_path.displayName(): nullptr;

        ui->fileName->setText(QString::fromStdString(fileInfo->name()));
        if(parent_str) {
            ui->location->setText(QString::fromUtf8(parent_str.get()));
        }
        else {
            ui->location->clear();
        }
        auto mtime = QDateTime::fromMSecsSinceEpoch(fileInfo->mtime() * 1000);
        ui->lastModified->setText(mtime.toString(Qt::SystemLocaleShortDate));
        auto atime = QDateTime::fromMSecsSinceEpoch(fileInfo->atime() * 1000);
        ui->lastAccessed->setText(atime.toString(Qt::SystemLocaleShortDate));
    }
    else {
        ui->fileName->setText(tr("Multiple Files"));
        ui->fileName->setEnabled(false);
    }

    initApplications(); // init applications combo box

    // calculate total file sizes
    fileSizeTimer = new QTimer(this);
    connect(fileSizeTimer, &QTimer::timeout, this, &FilePropsDialog::onFileSizeTimerTimeout);
    fileSizeTimer->start(600);

    connect(totalSizeJob, &Fm::TotalSizeJob::finished, this, &FilePropsDialog::onDeepCountJobFinished, Qt::BlockingQueuedConnection);
    totalSizeJob->setAutoDelete(true);
    totalSizeJob->runAsync();

    // disk usage
    bool canShowDeviceUsage = false;
    if(fileInfo->dirPath()) { // skip directories like "search:///"
        auto folder = Fm::Folder::fromPath(fileInfo->dirPath());
        if(!folder->isLoaded() && fileInfo->isDir()) { // an empty space is right clicked
            folder = Fm::Folder::fromPath(fileInfo->path());
        }
        guint64 free, total;
        if(folder->getFilesystemInfo(&total, &free)) {
            canShowDeviceUsage = true;
            ui->progressBar->setValue(qRound(static_cast<qreal>((total - free) * 100) / static_cast<qreal>(total)));
            ui->progressBar->setFormat(tr("%p% used"));
            ui->spaceLabel->setText(tr("%1 Free of %2")
                                    .arg(formatFileSize(free, false),
                                         formatFileSize(total, false)));
        }
    }
    if(!canShowDeviceUsage) {
        ui->deviceLabel->setVisible(false);
        ui->spaceLabel->setVisible(false);
        ui->progressBar->setVisible(false);
    }
}

void FilePropsDialog::onDeepCountJobFinished() {
    onFileSizeTimerTimeout(); // update file size display

    totalSizeJob = nullptr;

    // stop the timer
    if(fileSizeTimer) {
        fileSizeTimer->stop();
        delete fileSizeTimer;
        fileSizeTimer = nullptr;
    }
}

void FilePropsDialog::onFileSizeTimerTimeout() {
    if(totalSizeJob && !totalSizeJob->isCancelled()) {
        // FIXME:
        // OMG! It's really unbelievable that Qt developers only implement
        // QObject::tr(... int n). GNU gettext developers are smarter and
        // they use unsigned long instead of int.
        // We cannot use Qt here to handle plural forms. So sad. :-(
        QString str = Fm::formatFileSize(totalSizeJob->totalSize(), fm_config->si_unit) %
                      QStringLiteral(" (%1 B)").arg(totalSizeJob->totalSize());
        // tr(" (%n) byte(s)", "", deepCountJob->total_size);
        ui->fileSize->setText(str);

        str = Fm::formatFileSize(totalSizeJob->totalOnDiskSize(), fm_config->si_unit) %
              QStringLiteral(" (%1 B)").arg(totalSizeJob->totalOnDiskSize());
        // tr(" (%n) byte(s)", "", deepCountJob->total_ondisk_size);
        ui->onDiskSize->setText(str);

        if(ui->contentsLabel->isVisible()) {
            unsigned int fc =  totalSizeJob->fileCount(); // the directory is included
            if (fc <= 1)
                str = tr("no file");
            else if (fc == 2)
                str = tr("one file");
            else
                str = tr("%Ln files", "", fc - 1);
            ui->fileNumber->setText(str);
        }
    }
}

void FilePropsDialog::onIconButtonclicked() {
    QString iconDir;
    QString iconThemeName = QIcon::themeName();
    QStringList icons = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                  QStringLiteral("icons"),
                                                  QStandardPaths::LocateDirectory);
    for (QStringList::ConstIterator it = icons.constBegin(); it != icons.constEnd(); ++it) {
        QString iconThemeFolder = *it + QLatin1String("/") + iconThemeName;
        if (QDir(iconThemeFolder).exists() && QFileInfo(iconThemeFolder).permission(QFileDevice::ReadUser)) {
            // give priority to the "places" folder
            const QString places = iconThemeFolder + QLatin1String("/places");
            if (QDir(places).exists() && QFileInfo(places).permission(QFileDevice::ReadUser)) {
                iconDir = places;
            }
            else {
                iconDir = iconThemeFolder;
            }
            break;
        }
    }
    if(iconDir.isEmpty()) {
        iconDir = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                         QStringLiteral("icons"),
                                         QStandardPaths::LocateDirectory);
        if(iconDir.isEmpty()) {
            return;
        }
    }
    const QString iconPath = QFileDialog::getOpenFileName(this, tr("Select an icon"),
                                                          iconDir,
                                                          tr("Images (*.png *.xpm *.svg *.svgz )"));
    if(!iconPath.isEmpty()) {
        QStringList parts = iconPath.split(QStringLiteral("/"), QString::SkipEmptyParts);
        if(!parts.isEmpty()) {
            QString iconName = parts.at(parts.count() - 1);
            int ln = iconName.lastIndexOf(QLatin1String("."));
            if(ln > -1) {
                iconName.remove(ln, iconName.length() - ln);
                customIcon = QIcon::fromTheme(iconName);
                ui->iconButton->setIcon(customIcon);
            }
        }
    }
}

void FilePropsDialog::accept() {

    // applications
    if(mimeType && ui->openWith->isChanged()) {
        auto currentApp = ui->openWith->selectedApp();
        setDefaultAppForType(currentApp, mimeType);
    }

    // check if chown or chmod is needed
    uid_t newUid = uidFromName(ui->owner->text());
    gid_t newGid = gidFromName(ui->ownerGroup->text());
    bool needChown = (newUid != INVALID_UID && newUid != uid) || (newGid != INVALID_GID && newGid != gid);

    int newOwnerPermSel = ui->ownerPerm->currentIndex();
    int newGroupPermSel = ui->groupPerm->currentIndex();
    int newOtherPermSel = ui->otherPerm->currentIndex();
    Qt::CheckState newExecCheckState = ui->executable->checkState();
    bool needChmod = ((newOwnerPermSel != ownerPermSel) ||
                      (newGroupPermSel != groupPermSel) ||
                      (newOtherPermSel != otherPermSel) ||
                      (newExecCheckState != execCheckState));

    if(needChmod || needChown) {
        FileOperation* op = new FileOperation(FileOperation::ChangeAttr, fileInfos_.paths());
        if(needChown) {
            // don't do chown if new uid/gid and the original ones are actually the same.
            if(newUid == uid) {
                newUid = INVALID_UID;
            }
            if(newGid == gid) {
                newGid = INVALID_GID;
            }
            op->setChown(newUid, newGid);
        }
        if(needChmod) {
            mode_t newMode = 0;
            mode_t newModeMask = 0;
            // FIXME: we need to make sure that folders with "r" permission also have "x"
            // at the same time. Otherwise, it's not able to browse the folder later.
            if(newOwnerPermSel != ownerPermSel && newOwnerPermSel != ACCESS_NO_CHANGE) {
                // owner permission changed
                newModeMask |= (S_IRUSR | S_IWUSR); // affect user bits
                if(newOwnerPermSel == ACCESS_READ_ONLY) {
                    newMode |= S_IRUSR;
                }
                else if(newOwnerPermSel == ACCESS_READ_WRITE) {
                    newMode |= (S_IRUSR | S_IWUSR);
                }
            }
            if(newGroupPermSel != groupPermSel && newGroupPermSel != ACCESS_NO_CHANGE) {
                qDebug("newGroupPermSel: %d", newGroupPermSel);
                // group permission changed
                newModeMask |= (S_IRGRP | S_IWGRP); // affect group bits
                if(newGroupPermSel == ACCESS_READ_ONLY) {
                    newMode |= S_IRGRP;
                }
                else if(newGroupPermSel == ACCESS_READ_WRITE) {
                    newMode |= (S_IRGRP | S_IWGRP);
                }
            }
            if(newOtherPermSel != otherPermSel && newOtherPermSel != ACCESS_NO_CHANGE) {
                // other permission changed
                newModeMask |= (S_IROTH | S_IWOTH); // affect other bits
                if(newOtherPermSel == ACCESS_READ_ONLY) {
                    newMode |= S_IROTH;
                }
                else if(newOtherPermSel == ACCESS_READ_WRITE) {
                    newMode |= (S_IROTH | S_IWOTH);
                }
            }
            if(newExecCheckState != execCheckState && newExecCheckState != Qt::PartiallyChecked) {
                // executable state changed
                newModeMask |= (S_IXUSR | S_IXGRP | S_IXOTH);
                if(newExecCheckState == Qt::Checked) {
                    newMode |= (S_IXUSR | S_IXGRP | S_IXOTH);
                }
            }
            op->setChmod(newMode, newModeMask);

            if(hasDir) { // if there are some dirs in our selected files
                QMessageBox::StandardButton r = QMessageBox::question(this,
                                                tr("Apply changes"),
                                                tr("Do you want to recursively apply these changes to all files and sub-folders?"),
                                                QMessageBox::Yes | QMessageBox::No);
                if(r == QMessageBox::Yes) {
                    op->setRecursiveChattr(true);
                }
            }
        }
        op->setAutoDestroy(true);
        op->run();
    }

    // Renaming
    if(singleFile) {
        QString new_name = ui->fileName->text();
        if(QString::fromStdString(fileInfo->name()) != new_name) {
            auto path = fileInfo->path();
            auto parent_path = path.parent();
            auto dest = parent_path.child(new_name.toLocal8Bit().constData());
            Fm::GErrorPtr err;
            if(!g_file_move(path.gfile().get(), dest.gfile().get(),
                            GFileCopyFlags(G_FILE_COPY_ALL_METADATA |
                                           G_FILE_COPY_NO_FALLBACK_FOR_MOVE |
                                           G_FILE_COPY_NOFOLLOW_SYMLINKS),
                            nullptr, nullptr, nullptr, &err)) {
                QMessageBox::critical(this, QObject::tr("Error"), err.message());
            }
        }
    }

    // Custom (folder) icon
    if(!customIcon.isNull()) {
        bool reloadNeeded(false);
        QString iconNamne = customIcon.name();
        for(auto& fi: fileInfos_) {
            std::shared_ptr<const Fm::IconInfo> icon = fi->icon();
            if (!fi->icon() || fi->icon()->qicon().name() != iconNamne) {
                auto dot_dir = CStrPtr{g_build_filename(fi->path().localPath().get(), ".directory", nullptr)};
                GKeyFile* kf = g_key_file_new();
                g_key_file_set_string(kf, "Desktop Entry", "Icon", iconNamne.toLocal8Bit().constData());
                Fm::GErrorPtr err;
                if (!g_key_file_save_to_file(kf, dot_dir.get(), &err)) {
                    QMessageBox::critical(this, QObject::tr("Custom Icon Error"), err.message());
                }
                else {
                    reloadNeeded = true;
                }
                g_key_file_free(kf);
            }
        }
        if(reloadNeeded) {
            // since there can be only one parent dir, only one reload is needed
            auto parent = fileInfo->path().parent();
            if(parent.isValid()) {
                auto folder = Fm::Folder::fromPath(parent);
                if(folder->isLoaded()) {
                    folder->reload();
                }
            }
        }
    }

    QDialog::accept();
}

void FilePropsDialog::initOwner() {
    if(allNative) {
        if(uid != DIFFERENT_UIDS) {
            ui->owner->setText(uidToName(uid));
        }
        if(gid != DIFFERENT_GIDS) {
            ui->ownerGroup->setText(gidToName(gid));
        }

        if(geteuid() != 0) { // on local filesystems, only root can do chown.
            ui->owner->setEnabled(false);
            ui->ownerGroup->setEnabled(false);
        }
    }
}


} // namespace Fm
