/**
  * (c)LGPL2+
  * This file is part of the KDE project
  * Copyright (C) 2007, 2009 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef KCATEGORIZEDVIEW_H
#define KCATEGORIZEDVIEW_H

#include <QListView>

//#include <kdeui_export.h>

#define KDEUI_EXPORT
#define KDE_DEPRECATED


class QCategoryDrawer;
class QCategoryDrawerV2;

/**
  * @short Item view for listing items in a categorized fashion optionally
  *
  * QCategorizedView basically has the same functionality as QListView, only that it also lets you
  * layout items in a way that they are categorized visually.
  *
  * For it to work you will need to set a QCategorizedSortFilterProxyModel and a QCategoryDrawer
  * with methods setModel() and setCategoryDrawer() respectively. Also, the model will need to be
  * flagged as categorized with QCategorizedSortFilterProxyModel::setCategorizedModel(true).
  *
  * The way it works (if categorization enabled):
  *
  *     - When sorting, it does more things than QListView does. It will ask the model for the
  *       special role CategorySortRole (@see QCategorizedSortFilterProxyModel). This can return
  *       a QString or an int in order to tell the view the order of categories. In this sense, for
  *       instance, if we are sorting by name ascending, "A" would be before than "B". If we are
  *       sorting by size ascending, 512 bytes would be before 1024 bytes. This way categories are
  *       also sorted.
  *
  *     - When the view has to paint, it will ask the model with the role CategoryDisplayRole
  *       (@see QCategorizedSortFilterProxyModel). It will for instance return "F" for "foo.pdf" if
  *       we are sorting by name ascending, or "Small" if a certain item has 100 bytes, for example.
  *
  * For drawing categories, QCategoryDrawer will be used. You can inherit this class to do your own
  * drawing.
  *
  * @note All examples cited before talk about filesystems and such, but have present that this
  *       is a completely generic class, and it can be used for whatever your purpose is. For
  *       instance when talking about animals, you can separate them by "Mammal" and "Oviparous". In
  *       this very case, for example, the CategorySortRole and the CategoryDisplayRole could be the
  *       same ("Mammal" and "Oviparous").
  *
  * @note There is a really performance boost if CategorySortRole returns an int instead of a QString.
  *       Have present that this role is asked (n * log n) times when sorting and compared. Comparing
  *       ints is always faster than comparing strings, whithout mattering how fast the string
  *       comparison is. Consider thinking of a way of returning ints instead of QStrings if your
  *       model can contain a high number of items.
  *
  * @warning Note that for really drawing items in blocks you will need some things to be done:
  *             - The model set to this view has to be (or inherit if you want to do special stuff
  *               in it) QCategorizedSortFilterProxyModel.
  *             - This model needs to be set setCategorizedModel to true.
  *             - Set a category drawer by calling setCategoryDrawer.
  *
  * @see QCategorizedSortFilterProxyModel, QCategoryDrawer
  *
  * @author Rafael Fernández López <ereslibre@kde.org>
  */
class KDEUI_EXPORT QCategorizedView
    : public QListView
{
    Q_OBJECT
    Q_PROPERTY(int categorySpacing READ categorySpacing WRITE setCategorySpacing)
    Q_PROPERTY(bool alternatingBlockColors READ alternatingBlockColors WRITE setAlternatingBlockColors)
    Q_PROPERTY(bool collapsibleBlocks READ collapsibleBlocks WRITE setCollapsibleBlocks)

public:
    QCategorizedView(QWidget *parent = nullptr);

    ~QCategorizedView() override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void setModel(QAbstractItemModel *model) override;

    /**
      * Calls to setGridSizeOwn().
      */
    void setGridSize(const QSize &size);

    /**
      * @warning note that setGridSize is not virtual in the base class (QListView), so if you are
      *          calling to this method, make sure you have a QCategorizedView pointer around. This
      *          means that something like:
      * @code
      *     QListView *lv = new QCategorizedView();
      *     lv->setGridSize(mySize);
      * @endcode
      *
      * will not call to the expected setGridSize method. Instead do something like this:
      *
      * @code
      *     QListView *lv;
      *     ...
      *     QCategorizedView *cv = qobject_cast<QCategorizedView*>(lv);
      *     if (cv) {
      *         cv->setGridSizeOwn(mySize);
      *     } else {
      *         lv->setGridSize(mySize);
      *     }
      * @endcode
      *
      * @note this method will call to QListView::setGridSize among other operations.
      *
      * @since 4.4
      */
    void setGridSizeOwn(const QSize &size);

    /**
      * Reimplemented from QAbstractItemView.
      */
    QRect visualRect(const QModelIndex &index) const override;

    /**
      * Returns the current category drawer.
      */
    QCategoryDrawer *categoryDrawer() const;

    /**
      * The category drawer that will be used for drawing categories.
      */
    void setCategoryDrawer(QCategoryDrawer *categoryDrawer);

    /**
      * @return Category spacing. The spacing between categories.
      *
      * @since 4.4
      */
    int categorySpacing() const;

    /**
      * Stablishes the category spacing. This is the spacing between categories.
      *
      * @since 4.4
      */
    void setCategorySpacing(int categorySpacing);

    /**
      * @return Whether blocks should be drawn with alternating colors.
      *
      * @since 4.4
      */
    bool alternatingBlockColors() const;

    /**
      * Sets whether blocks should be drawn with alternating colors.
      *
      * @since 4.4
      */
    void setAlternatingBlockColors(bool enable);

    /**
      * @return Whether blocks can be collapsed or not.
      *
      * @since 4.4
      */
    bool collapsibleBlocks() const;

    /**
      * Sets whether blocks can be collapsed or not.
      *
      * @since 4.4
      */
    void setCollapsibleBlocks(bool enable);

    /**
      * @return Block of indexes that are into @p category.
      *
      * @since 4.5
      */
    QModelIndexList block(const QString &category);

    /**
      * @return Block of indexes that are represented by @p representative.
      *
      * @since 4.5
      */
    QModelIndexList block(const QModelIndex &representative);

    /**
      * Reimplemented from QAbstractItemView.
      */
    QModelIndex indexAt(const QPoint &point) const override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void reset() override;

    /**
     * @return The icon size by considering all the styling
     */
    QSize decorationSize() const;

protected:
    /**
      * Reimplemented from QWidget.
      */
    void paintEvent(QPaintEvent *event) override;

    /**
      * Reimplemented from QWidget.
      */
    void resizeEvent(QResizeEvent *event) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void setSelection(const QRect &rect,
                              QItemSelectionModel::SelectionFlags flags) override;

    /**
      * Reimplemented from QWidget.
      */
    void mouseMoveEvent(QMouseEvent *event) override;

    /**
      * Reimplemented from QWidget.
      */
    void mousePressEvent(QMouseEvent *event) override;

    /**
      * Reimplemented from QWidget.
      */
    void mouseReleaseEvent(QMouseEvent *event) override;

    /**
      * Reimplemented from QWidget.
      */
    void leaveEvent(QEvent *event) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void startDrag(Qt::DropActions supportedActions) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void dragMoveEvent(QDragMoveEvent *event) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void dragLeaveEvent(QDragLeaveEvent *event) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void dropEvent(QDropEvent *event) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    QModelIndex moveCursor(CursorAction cursorAction,
                                   Qt::KeyboardModifiers modifiers) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void rowsAboutToBeRemoved(const QModelIndex &parent,
                                      int start,
                                      int end) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void updateGeometries() override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void currentChanged(const QModelIndex &current,
                                const QModelIndex &previous) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void dataChanged(const QModelIndex &topLeft,
                             const QModelIndex &bottomRight,
                             const QVector<int> & roles = QVector<int> ()) override;

    /**
      * Reimplemented from QAbstractItemView.
      */
    void rowsInserted(const QModelIndex &parent,
                              int start,
                              int end) override;

protected Q_SLOTS:
    /**
      * @internal
      * @warning Deprecated since 4.4.
      */
#ifndef KDE_NO_DEPRECATED
    virtual KDE_DEPRECATED void rowsInsertedArtifficial(const QModelIndex &parent,
                                                        int start,
                                                        int end);
#endif

    /**
      * @internal
      * @warning Deprecated since 4.4.
      */
#ifndef KDE_NO_DEPRECATED
    virtual KDE_DEPRECATED void rowsRemoved(const QModelIndex &parent,
                                            int start,
                                            int end);
#endif

    /**
      * @internal
      * Reposition items as needed.
      */
    virtual void slotLayoutChanged();

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void _k_slotCollapseOrExpandClicked(QModelIndex))
};

#endif // KCATEGORIZEDVIEW_H
