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


#ifndef FM_PLACESVIEW_H
#define FM_PLACESVIEW_H

#include "libfmqtglobals.h"
#include <QTreeView>
#include <QSortFilterProxyModel>

#include <memory>
#include "core/filepath.h"

namespace Fm {

class PlacesModel;
class PlacesModelItem;
class PlacesView;

class LIBFM_QT_API PlacesProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit PlacesProxyModel(QObject* parent = nullptr);
    ~PlacesProxyModel() override;

    void setHidden(const QString& str, bool hide = true);

    void restoreHiddenItems(const QSet<QString>& items);

    void showAll(bool show);

    bool isShowingAll() const {
        return showAll_;
    }

    bool isHidden(const QString& str) const {
        return hidden_.contains(str);
    }

    bool hasHidden() const {
        return !hidden_.isEmpty();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;

private:
    QSet<QString> hidden_;
    bool showAll_;
    bool hiddenItemsRestored_;
};

class LIBFM_QT_API PlacesView : public QTreeView {
    Q_OBJECT

public:
    explicit PlacesView(QWidget* parent = nullptr);
    ~PlacesView() override;

    void setCurrentPath(Fm::FilePath path);

    const Fm::FilePath& currentPath() const {
        return currentPath_;
    }

    void chdir(Fm::FilePath path) {
        setCurrentPath(std::move(path));
    }

    void restoreHiddenItems(const QSet<QString>& items);

Q_SIGNALS:
    void chdirRequested(int type, const Fm::FilePath& path);
    void hiddenItemSet(const QString& str, bool hide);

protected Q_SLOTS:
    void onClicked(const QModelIndex& index);
    void onPressed(const QModelIndex& index);
    void onIconSizeChanged(const QSize& size);
    // void onMountOperationFinished(GError* error);

    void onOpenNewTab();
    void onOpenNewWindow();

    void onEmptyTrash();

    void onMountVolume();
    void onUnmountVolume();
    void onEjectVolume();
    void onUnmountMount();

    void onMoveBookmarkUp();
    void onMoveBookmarkDown();
    void onDeleteBookmark();
    void onRenameBookmark();

protected:
    void drawBranches(QPainter* /*painter*/, const QRect& /*rect*/, const QModelIndex& /*index*/) const override {
        // override this method to inhibit drawing of the branch grid lines by Qt.
    }

    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void commitData(QWidget* editor) override;

private:
    void onEjectButtonClicked(PlacesModelItem* item);
    void activateRow(int type, const QModelIndex& index);
    void showAll(bool show);
    void spanFirstColumn();

private:
    std::shared_ptr<PlacesModel> model_;
    Fm::FilePath currentPath_;

    static std::shared_ptr<PlacesProxyModel> proxyModel_; // used to hide items in all views
};

}

#endif // FM_PLACESVIEW_H
