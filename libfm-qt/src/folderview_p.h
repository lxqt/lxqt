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


#ifndef FM_FOLDERVIEW_P_H
#define FM_FOLDERVIEW_P_H

#include <QListView>
#include <QTreeView>
#include <QMouseEvent>
#include "folderview.h"

class QTimer;

namespace Fm {

// override these classes for implementing FolderView
class FolderViewListView : public QListView {
  Q_OBJECT
public:
  friend class FolderView;
  FolderViewListView(QWidget* parent = nullptr);
  ~FolderViewListView() override;
  void startDrag(Qt::DropActions supportedActions) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* e) override;
  void dragLeaveEvent(QDragLeaveEvent* e) override;
  void dropEvent(QDropEvent* e) override;

  QModelIndex indexAt(const QPoint & point) const override;

  inline void setPositionForIndex(const QPoint & position, const QModelIndex & index) {
    QListView::setPositionForIndex(position, index);
  }

  inline QRect rectForIndex(const QModelIndex & index) const {
    return QListView::rectForIndex(index);
  }

  inline QStyleOptionViewItem getViewOptions() {
    return viewOptions();
  }

  inline bool cursorOnSelectionCorner() const {
      return cursorOnSelectionCorner_;
  }

Q_SIGNALS:
  void activatedFiltered(const QModelIndex &index);

protected:
  QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;

public Q_SLOTS:
  void selectAll() override;

private Q_SLOTS:
  void activation(const QModelIndex &index);

private:
  bool activationAllowed_;
  mutable bool cursorOnSelectionCorner_;
  bool mouseLeftPressed_;
  QPoint globalItemPressPoint_; // to prevent dragging when only the view is scrolled
};

class FolderViewTreeView : public QTreeView {
  Q_OBJECT
public:
  friend class FolderView;
  FolderViewTreeView(QWidget* parent = nullptr);
  ~FolderViewTreeView() override;
  void setModel(QAbstractItemModel* model) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void dragEnterEvent(QDragEnterEvent* event) override;
  void dragMoveEvent(QDragMoveEvent* e) override;
  void dragLeaveEvent(QDragLeaveEvent* e) override;
  void dropEvent(QDropEvent* e) override;

  // for rubberband
  void paintEvent(QPaintEvent * event) override;
  void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override;

  void rowsInserted(const QModelIndex& parent,int start, int end) override;
  void rowsAboutToBeRemoved(const QModelIndex& parent,int start, int end) override;
  void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>{}) override;
  void reset() override;

  void resizeEvent(QResizeEvent* event) override;
  void queueLayoutColumns();

  void keyboardSearch(const QString &search) override {
    QAbstractItemView::keyboardSearch(search); // let items be selected by typing
  }

  void setCustomColumnWidths(const QList<int> &widths);

  void setHiddenColumns(const QSet<int> &columns);

Q_SIGNALS:
  void activatedFiltered(const QModelIndex &index);
  void columnResizedByUser(int visualIndex, int newWidth);
  void autoResizeEnabled();
  void columnHiddenByUser(int visibleIndex, bool hidden);

private Q_SLOTS:
  void layoutColumns();
  void activation(const QModelIndex &index);
  void onSortFilterChanged();
  void headerContextMenu(const QPoint &p);

private:
  bool doingLayout_;
  QTimer* layoutTimer_;
  bool activationAllowed_;
  QList<int> customColumnWidths_;
  QSet<int> hiddenColumns_;
  QPoint globalItemPressPoint_; // to prevent dragging when only the view is scrolled

  // for rubberband
  QPoint mousePressPoint_;
  QRect rubberBandRect_;
  QItemSelectionModel::SelectionFlag ctrlDragSelectionFlag_;
};


} // namespace Fm

#endif // FM_FOLDERVIEW_P_H
