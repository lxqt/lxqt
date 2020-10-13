/*
 * Copyright (C) 2014 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef FM_DIRTREEVIEW_H
#define FM_DIRTREEVIEW_H

#include "libfmqtglobals.h"
#include <QTreeView>

#include "core/filepath.h"

class QItemSelection;

namespace Fm {

class FileMenu;
class DirTreeModelItem;

class LIBFM_QT_API DirTreeView : public QTreeView {
    Q_OBJECT

public:
    explicit DirTreeView(QWidget* parent);
    ~DirTreeView() override;

    const Fm::FilePath& currentPath() const {
        return currentPath_;
    }

    void setCurrentPath(Fm::FilePath path);

    void chdir(Fm::FilePath path) {
        setCurrentPath(std::move(path));
    }

    void setModel(QAbstractItemModel* model) override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) override;

private:
    void cancelPendingChdir();
    void expandPendingPath();

Q_SIGNALS:
    void chdirRequested(int type, const Fm::FilePath& path);
    void openFolderInNewWindowRequested(const Fm::FilePath& path);
    void openFolderInNewTabRequested(const Fm::FilePath& path);
    void openFolderInTerminalRequested(const Fm::FilePath& path);
    void createNewFolderRequested(const Fm::FilePath& path);
    void prepareFileMenu(Fm::FileMenu* menu); // emit before showing a Fm::FileMenu

protected Q_SLOTS:
    void onCollapsed(const QModelIndex& index);
    void onExpanded(const QModelIndex& index);
    void onRowLoaded(const QModelIndex& index);
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onCustomContextMenuRequested(const QPoint& pos);
    void onOpen();
    void onNewWindow();
    void onNewTab();
    void onOpenInTerminal();
    void onNewFolder();
    void rowsRemoved(const QModelIndex& parent, int start, int end);
    void doQueuedDeletions();

private:
    Fm::FilePath currentPath_;
    Fm::FilePathList pathsToExpand_;
    DirTreeModelItem* currentExpandingItem_;
    std::vector<DirTreeModelItem*> queuedForDeletion_;
};

}

#endif // FM_DIRTREEVIEW_H
