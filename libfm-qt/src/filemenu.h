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


#ifndef FM_FILEMENU_H
#define FM_FILEMENU_H

#include "libfmqtglobals.h"
#include <QMenu>
#include <qabstractitemmodel.h>
#include "core/fileinfo.h"

class QAction;

namespace Fm {

class FileLauncher;
class FileActionItem;

class LIBFM_QT_API FileMenu : public QMenu {
    Q_OBJECT

public:
    explicit FileMenu(Fm::FileInfoList files, std::shared_ptr<const Fm::FileInfo> info, Fm::FilePath cwd, bool isWritableDir = true, const QString& title = QString(), QWidget* parent = nullptr);
    ~FileMenu() override;

    void addTrustAction();

    bool useTrash() {
        return useTrash_;
    }

    void setUseTrash(bool trash);

    bool confirmDelete() {
        return confirmDelete_;
    }

    void setConfirmDelete(bool confirm) {
        confirmDelete_ = confirm;
    }

    QAction* openAction() {
        return openAction_;
    }

    QAction* openWithMenuAction() {
        return openWithMenuAction_;
    }

    QAction* openWithAction() {
        return openWithAction_;
    }

    QAction* separator1() {
        return separator1_;
    }

    QAction* createAction() {
        return createAction_;
    }

    QAction* separator2() {
        return separator2_;
    }

    QAction* cutAction() {
        return cutAction_;
    }

    QAction* copyAction() {
        return copyAction_;
    }

    QAction* pasteAction() {
        return pasteAction_;
    }

    QAction* deleteAction() {
        return deleteAction_;
    }

    QAction* unTrashAction() {
        return unTrashAction_;
    }

    QAction* renameAction() {
        return renameAction_;
    }

    QAction* separator3() {
        return separator3_;
    }

    QAction* propertiesAction() {
        return propertiesAction_;
    }

    const Fm::FileInfoList& files() const {
        return files_;
    }

    const std::shared_ptr<const Fm::FileInfo>& firstFile() const {
        return info_;
    }

    const Fm::FilePath& cwd() const {
        return cwd_;
    }

    void setFileLauncher(FileLauncher* launcher) {
        fileLauncher_ = launcher;
    }

    FileLauncher* fileLauncher() {
        return fileLauncher_;
    }

    bool sameType() const {
        return sameType_;
    }

    bool sameFilesystem() const {
        return sameFilesystem_;
    }

    bool allVirtual() const {
        return allVirtual_;
    }

    bool allTrash() const {
        return allTrash_;
    }

    bool confirmTrash() const {
        return confirmTrash_;
    }

    void setConfirmTrash(bool value) {
        confirmTrash_ = value;
    }

protected:
    void addCustomActionItem(QMenu* menu, std::shared_ptr<const FileActionItem> item);
    void openFilesWithApp(GAppInfo* app);

protected Q_SLOTS:
    void onOpenTriggered();
    void onOpenWithTriggered();
    void onTrustToggled(bool checked);
    void onFilePropertiesTriggered();
    void onApplicationTriggered();
    void onCustomActionTrigerred();
    void onCompress();
    void onExtract();
    void onExtractHere();

    void onCutTriggered();
    void onCopyTriggered();
    void onPasteTriggered();
    void onRenameTriggered();
    void onDeleteTriggered();
    void onUnTrashTriggered();

private:
    Fm::FileInfoList files_;
    std::shared_ptr<const Fm::FileInfo> info_;
    Fm::FilePath cwd_;
    bool useTrash_;
    bool confirmDelete_;
    bool confirmTrash_; // Confirm before moving files into "trash can"

    bool sameType_;
    bool sameFilesystem_;
    bool allVirtual_;
    bool allTrash_;

    QAction* openAction_;
    QAction* openWithMenuAction_;
    QAction* openWithAction_;
    QAction* separator1_;
    QAction* createAction_;
    QAction* separator2_;
    QAction* cutAction_;
    QAction* copyAction_;
    QAction* pasteAction_;
    QAction* deleteAction_;
    QAction* unTrashAction_;
    QAction* renameAction_;
    QAction* separator3_;
    QAction* propertiesAction_;

    FileLauncher* fileLauncher_;
};

}

#endif // FM_FILEMENU_H
