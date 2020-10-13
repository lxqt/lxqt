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

#include "dirtreeview.h"
#include <QHeaderView>
#include <QDebug>
#include <QItemSelection>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QTimer>
#include "dirtreemodel.h"
#include "dirtreemodelitem.h"
#include "filemenu.h"

namespace Fm {

DirTreeView::DirTreeView(QWidget* parent):
    QTreeView(parent),
    currentExpandingItem_(nullptr) {

    setSelectionMode(QAbstractItemView::SingleSelection);
    setHeaderHidden(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    header()->setStretchLastSection(false);

    connect(this, &DirTreeView::collapsed, this, &DirTreeView::onCollapsed);
    connect(this, &DirTreeView::expanded, this, &DirTreeView::onExpanded);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &DirTreeView::customContextMenuRequested,
            this, &DirTreeView::onCustomContextMenuRequested);

    setAcceptDrops(true);
}

DirTreeView::~DirTreeView() {
}

void DirTreeView::cancelPendingChdir() {
    if(!pathsToExpand_.empty()) {
        pathsToExpand_.clear();
        if(!currentExpandingItem_) {
            return;
        }
        DirTreeModel* _model = static_cast<DirTreeModel*>(model());
        disconnect(_model, &DirTreeModel::rowLoaded, this, &DirTreeView::onRowLoaded);
        currentExpandingItem_ = nullptr;
    }
}

void DirTreeView::expandPendingPath() {
    if(pathsToExpand_.empty()) {
        return;
    }

    auto path = pathsToExpand_.front();
    // qDebug() << "expanding: " << Path(path).displayBasename();
    DirTreeModel* _model = static_cast<DirTreeModel*>(model());
    DirTreeModelItem* item = _model->itemFromPath(path);
    // qDebug() << "findItem: " << item;
    if(item) {
        currentExpandingItem_ = item;
        connect(_model, &DirTreeModel::rowLoaded, this, &DirTreeView::onRowLoaded);
        if(item->loaded_) { // the node is already loaded
            onRowLoaded(item->index());
        }
        else {
            // _model->loadRow(item->index());
            item->loadFolder();
        }
    }
    else {
        selectionModel()->clear();
        /* since we never get it loaded we need to update cwd here */
        currentPath_ = path;

        cancelPendingChdir(); // FIXME: is this correct? this is not done in the gtk+ version of libfm.
    }
}

void DirTreeView::onRowLoaded(const QModelIndex& index) {
    DirTreeModel* _model = static_cast<DirTreeModel*>(model());
    if(!currentExpandingItem_) {
        return;
    }
    if(currentExpandingItem_ != _model->itemFromIndex(index)) {
        return;
    }
    /* disconnect the handler since we only need it once */
    disconnect(_model, &DirTreeModel::rowLoaded, this, &DirTreeView::onRowLoaded);

    // DirTreeModelItem* item = _model->itemFromIndex(index);
    // qDebug() << "row loaded: " << item->displayName_;
    /* after the folder is loaded, the files should have been added to
      * the tree model */
    expand(index);

    /* remove the expanded path from pending list */
    pathsToExpand_.erase(pathsToExpand_.begin());
    if(pathsToExpand_.empty()) {  /* this is the last one and we're done, select the item */
        // qDebug() << "Done!";
        selectionModel()->select(index, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Clear);
        scrollTo(index, QAbstractItemView::EnsureVisible);
    }
    else { /* continue expanding next pending path */
        expandPendingPath();
    }
}


void DirTreeView::setCurrentPath(Fm::FilePath path) {
    DirTreeModel* _model = static_cast<DirTreeModel*>(model());
    if(!_model) {
        return;
    }
    int rowCount = _model->rowCount(QModelIndex());
    if(rowCount <= 0 || currentPath_ == path) {
        return;
    }

    currentPath_ = std::move(path);

    // NOTE: The content of each node is loaded on demand dynamically.
    // So, when we ask for a chdir operation, some nodes do not exists yet.
    // We have to wait for the loading of child nodes and continue the
    // pending chdir operation after the child nodes become available.

    // cancel previous pending tree expansion
    cancelPendingChdir();

    /* find a root item containing this path */
    Fm::FilePath root;
    for(int row = 0; row < rowCount; ++row) {
        QModelIndex index = _model->index(row, 0, QModelIndex());
        auto row_path = _model->filePath(index);
        if(row_path.isPrefixOf(currentPath_)) {
            root = row_path;
            break;
        }
    }

    if(root) { /* root item is found */
        path = currentPath_;
        do { /* add path elements one by one to a list */
            pathsToExpand_.insert(pathsToExpand_.cbegin(), path);
            // qDebug() << "prepend path: " << Path(path).displayBasename();
            if(path == root) {
                break;
            }
            path = path.parent();
        }
        while(path);

        expandPendingPath();
    }
}

void DirTreeView::setModel(QAbstractItemModel* model) {
    Q_ASSERT(model->inherits("Fm::DirTreeModel"));

    if(!pathsToExpand_.empty()) { // if a chdir request is in progress, cancel it
        cancelPendingChdir();
    }

    QTreeView::setModel(model);
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, &DirTreeView::onSelectionChanged);
}

void DirTreeView::mousePressEvent(QMouseEvent* event) {
    if(event && event->button() == Qt::RightButton &&
            event->type() == QEvent::MouseButtonPress) {
        // Do not change the selection when the context menu is activated.
        return;
    }
    QTreeView::mousePressEvent(event);
}

void DirTreeView::onCustomContextMenuRequested(const QPoint& pos) {
    QModelIndex index = indexAt(pos);
    if(index.isValid()) {
        QVariant data = index.data(DirTreeModel::FileInfoRole);
        auto fileInfo = data.value<std::shared_ptr<const Fm::FileInfo>>();
        if(fileInfo) {
            auto path = fileInfo->path();
            Fm::FileInfoList files ;
            files.push_back(fileInfo);
            Fm::FileMenu* menu = new Fm::FileMenu(files, fileInfo, path);
            // FIXME: apply some settings to the menu and set a proper file launcher to it
            Q_EMIT prepareFileMenu(menu);

            QVariant pathData = qVariantFromValue<Fm::FilePath>(path);
            QAction* action = menu->openAction();
            action->disconnect();
            action->setData(index);
            connect(action, &QAction::triggered, this, &DirTreeView::onOpen);
            action = new QAction(QIcon::fromTheme(QStringLiteral("window-new")), tr("Open in New T&ab"), menu);
            action->setData(pathData);
            connect(action, &QAction::triggered, this, &DirTreeView::onNewTab);
            menu->insertAction(menu->separator1(), action);
            action = new QAction(QIcon::fromTheme(QStringLiteral("window-new")), tr("Open in New Win&dow"), menu);
            action->setData(pathData);
            connect(action, &QAction::triggered, this, &DirTreeView::onNewWindow);
            menu->insertAction(menu->separator1(), action);
            if(fileInfo->isNative()) {
                action = new QAction(QIcon::fromTheme(QStringLiteral("utilities-terminal")), tr("Open in Termina&l"), menu);
                action->setData(pathData);
                connect(action, &QAction::triggered, this, &DirTreeView::onOpenInTerminal);
                menu->insertAction(menu->separator1(), action);
            }
            menu->exec(mapToGlobal(pos));
            delete menu;
        }
    }
}

void DirTreeView::onOpen() {
    if(QAction* action = qobject_cast<QAction*>(sender())) {
        setCurrentIndex(action->data().toModelIndex());
    }
}

void DirTreeView::onNewWindow() {
    if(QAction* action = qobject_cast<QAction*>(sender())) {
        auto path = action->data().value<Fm::FilePath>();
        Q_EMIT openFolderInNewWindowRequested(path);
    }
}

void DirTreeView::onNewTab() {
    if(QAction* action = qobject_cast<QAction*>(sender())) {
        auto path = action->data().value<Fm::FilePath>();
        Q_EMIT openFolderInNewTabRequested(path);
    }
}

void DirTreeView::onOpenInTerminal() {
    if(QAction* action = qobject_cast<QAction*>(sender())) {
        auto path = action->data().value<Fm::FilePath>();
        Q_EMIT openFolderInTerminalRequested(path);
    }
}

void DirTreeView::onNewFolder() {
    if(QAction* action = qobject_cast<QAction*>(sender())) {
        auto path = action->data().value<Fm::FilePath>();
        Q_EMIT createNewFolderRequested(path);
    }
}

void DirTreeView::onCollapsed(const QModelIndex& index) {
    DirTreeModel* treeModel = static_cast<DirTreeModel*>(model());
    if(treeModel) {
        treeModel->unloadRow(index);
    }
}

void DirTreeView::onExpanded(const QModelIndex& index) {
    DirTreeModel* treeModel = static_cast<DirTreeModel*>(model());
    if(treeModel) {
        treeModel->loadRow(index);
    }
}
void DirTreeView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
    // see if to-be-removed items are queued for deletion
    // and also clear selection if one of them is selected (otherwise a freeze will occur)
    QModelIndex selIndex;
    if(selectionModel()->selectedRows().size() == 1) {
        selIndex = selectionModel()->selectedRows().at(0);
    }
    for (int i = start; i <= end; ++i) {
        QModelIndex index = model()->index(i, 0, parent);
        if(index.isValid()) {
            if(index == selIndex) {
                selectionModel()->clear();
            }
            DirTreeModelItem* item = reinterpret_cast<DirTreeModelItem*>(index.internalPointer());
            if (item->isQueuedForDeletion()) {
                queuedForDeletion_.push_back(item);
            }
        }
    }

    QTreeView::rowsAboutToBeRemoved (parent, start, end);
}

void DirTreeView::rowsRemoved(const QModelIndex& parent, int start, int end) {
    QTreeView::rowsRemoved (parent, start, end);
    // do the queued deletions only after all rows are removed (otherwise a freeze might occur)
    QTimer::singleShot(0, this, SLOT (doQueuedDeletions()));
}

void DirTreeView::doQueuedDeletions() {
    if(!queuedForDeletion_.empty()) {
        for(DirTreeModelItem* const item : qAsConst(queuedForDeletion_)) {
            delete item;
        }
        queuedForDeletion_.clear();
    }
}

void DirTreeView::onSelectionChanged(const QItemSelection& selected, const QItemSelection& /*deselected*/) {
    if(!selected.isEmpty()) {
        QModelIndex index = selected.first().topLeft();
        DirTreeModel* _model = static_cast<DirTreeModel*>(model());
        auto path = _model->filePath(index);
        if(path && currentPath_ && path == currentPath_) {
            return;
        }
        cancelPendingChdir();
        if(!path) {
            return;
        }
        currentPath_ = std::move(path);

        // FIXME: use enums for type rather than hard-coded values 0 or 1
        int type = 0;
        if(QGuiApplication::mouseButtons() & Qt::MiddleButton) {
            type = 1;
        }
        Q_EMIT chdirRequested(type, currentPath_);
    }
}


} // namespace Fm
