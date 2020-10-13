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


#ifndef FM_FOLDERVIEW_H
#define FM_FOLDERVIEW_H

#include "libfmqtglobals.h"
#include <QWidget>
#include <QListView>
#include <QTreeView>
#include <QMouseEvent>
#include "foldermodel.h"
#include "proxyfoldermodel.h"

#include "core/folder.h"

class QTimer;

namespace Fm {

class FileMenu;
class FolderMenu;
class FileLauncher;
class FolderViewStyle;

class LIBFM_QT_API FolderView : public QWidget {
    Q_OBJECT

public:

    enum ViewMode {
        FirstViewMode = 1,
        IconMode = FirstViewMode,
        CompactMode,
        DetailedListMode,
        ThumbnailMode,
        LastViewMode = ThumbnailMode,
        NumViewModes = (LastViewMode - FirstViewMode + 1)
    };

    enum ClickType {
        ActivatedClick,
        MiddleClick,
        ContextMenuClick
    };

    friend class FolderViewTreeView;
    friend class FolderViewListView;

    explicit FolderView(ViewMode _mode = IconMode, QWidget* parent = nullptr);

    explicit FolderView(QWidget* parent): FolderView{IconMode, parent} {}

    ~FolderView() override;

    void setViewMode(ViewMode _mode);
    ViewMode viewMode() const;

    void setIconSize(ViewMode mode, QSize size);
    QSize iconSize(ViewMode mode) const;

    QAbstractItemView* childView() const;

    ProxyFolderModel* model() const;
    void setModel(ProxyFolderModel* _model);

    std::shared_ptr<Fm::Folder> folder() const {
        return model_ ? static_cast<FolderModel*>(model_->sourceModel())->folder() : nullptr;
    }

    std::shared_ptr<const Fm::FileInfo> folderInfo() const {
        auto _folder = folder();
        return _folder ? _folder->info() : nullptr;
    }

    Fm::FilePath path() {
        auto _folder = folder();
        return _folder ? _folder->path() : Fm::FilePath();
    }

    QItemSelectionModel* selectionModel() const;
    Fm::FileInfoList selectedFiles() const;
    Fm::FilePathList selectedFilePaths() const;
    bool hasSelection() const;
    QModelIndex indexFromFolderPath(const Fm::FilePath& folderPath) const;
    void selectFiles(const Fm::FileInfoList& files, bool add = false);

    void selectAll();

    void invertSelection();

    void setFileLauncher(FileLauncher* launcher) {
        fileLauncher_ = launcher;
    }

    FileLauncher* fileLauncher() {
        return fileLauncher_;
    }

    int autoSelectionDelay() const {
        return autoSelectionDelay_;
    }

    void setAutoSelectionDelay(int delay);

    void setShadowHidden(bool shadowHidden);

    void setCtrlRightClick(bool ctrlRightClick);

    QList<int> getCustomColumnWidths() const {
        return customColumnWidths_;
    }
    void setCustomColumnWidths(const QList<int> &widths);

    QList<int> getHiddenColumns() const {
        return hiddenColumns_.toList();
    }
    void setHiddenColumns(const QList<int> &columns);

protected:
    bool event(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    virtual void childMousePressEvent(QMouseEvent* event);
    virtual void childDragEnterEvent(QDragEnterEvent* event);
    virtual void childDragMoveEvent(QDragMoveEvent* e);
    virtual void childDragLeaveEvent(QDragLeaveEvent* e);
    virtual void childDropEvent(QDropEvent* e);

    void emitClickedAt(ClickType type, const QPoint& pos);

    QModelIndexList selectedRows(int column = 0) const;
    QModelIndexList selectedIndexes() const;

    virtual void prepareFileMenu(Fm::FileMenu* menu);
    virtual void prepareFolderMenu(Fm::FolderMenu* menu);

    bool eventFilter(QObject* watched, QEvent* event) override;

    void updateGridSize(); // called when view mode, icon size, font size or cell margin is changed

    QSize getMargins() const {
        return itemDelegateMargins_;
    }

    // sets the cell margins in the icon and thumbnail modes
    // and calls updateGridSize() when needed
    void setMargins(QSize size);

public Q_SLOTS:
    void onItemActivated(QModelIndex index);
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    virtual void onFileClicked(int type, const std::shared_ptr<const Fm::FileInfo>& fileInfo);

private Q_SLOTS:
    void onAutoSelectionTimeout();
    void onSelChangedTimeout();
    void onClosingEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint);
    void scrollSmoothly();

Q_SIGNALS:
    void clicked(int type, const std::shared_ptr<const Fm::FileInfo>& file);
    void clickedBack();
    void clickedForward();
    void selChanged();
    void sortChanged();

    void columnResizedByUser();
    void columnHiddenByUser();

private:

    QAbstractItemView* view;
    ProxyFolderModel* model_;
    ViewMode mode;
    QSize iconSize_[NumViewModes];
    FileLauncher* fileLauncher_;
    int autoSelectionDelay_;
    QTimer* autoSelectionTimer_;
    QModelIndex lastAutoSelectionIndex_;
    QTimer* selChangedTimer_;
    // the cell margins in the icon and thumbnail modes
    QSize itemDelegateMargins_;
    bool shadowHidden_;
    bool ctrlRightClick_; // show folder context menu with Ctrl + right click

    // smooth scrolling:
    struct scrollData {
        int delta;
        int leftFrames;
    };
    QList<scrollData> queuedScrollSteps_;
    QTimer *smoothScrollTimer_;

    QList<int> customColumnWidths_;
    QSet<int> hiddenColumns_;
};

}

#endif // FM_FOLDERVIEW_H
