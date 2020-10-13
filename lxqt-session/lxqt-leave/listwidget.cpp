/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2017 LXQt team
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "listwidget.h"
#include <QItemDelegate>
#include <QApplication>
#include <QKeyEvent>

/*!
 * This private delegate does:
 * - returns unified sizeHint() -> maximum of all items in (list) model
 * - cahes the sizeHint() to not iterate over all items and checking their size
 * - overrides decoration position to Qt::Top
 * - gives the items margins (increasing sizeHint()) ~~and mimics Button visual~~
 * - overrides painting the focus around the whole item (with the decoration)
 *
 * \note It is a single purpose delegate and expects, that the model
 * never changes (cached sizeHint() is never invalidated).
 */
class ItemDelegate : public QItemDelegate
{
public:
    static constexpr QMargins MARGINS{5, 5, 5, 5};
public:
    using QItemDelegate::QItemDelegate;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (mItemSize.isValid())
            return mItemSize;

        // compute maximum item size
        QStyleOptionViewItem opt = option;
        opt.decorationPosition = QStyleOptionViewItem::Top;
        QAbstractListModel const * model = qobject_cast<QAbstractListModel const *>(index.model());
        for (QModelIndex i = model->index(0); i.isValid(); i = model->index(i.row() + 1))
        {
            mItemSize = mItemSize.expandedTo(QItemDelegate::sizeHint(opt, i));
        }
        mItemSize += {MARGINS.left() + MARGINS.right(), MARGINS.top() + MARGINS.bottom()}; // add some margins
        return mItemSize;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        /*
        // mimic the button visual
        QStyleOption button_option;
        button_option.initFrom(option.widget);
        button_option.rect = option.rect;
        if (!(option.state & QStyle::State_HasFocus))
            button_option.state &= ~QStyle::State_HasFocus;
        QStyle * style = option.widget->style() ? option.widget->style() : QApplication::style();
        style->drawPrimitive(QStyle::PE_PanelButtonTool, &button_option, painter, option.widget);
        */
        QStyleOptionViewItem opt = option;
        opt.decorationPosition = QStyleOptionViewItem::Top;
        opt.displayAlignment = Qt::AlignHCenter | Qt::AlignTop;
        return QItemDelegate::paint(painter, opt, index);
    }

protected:
    // Note: We want to paint the focus rectangle around the whole (text+icon)
    //  (default in QItemDelegate is to draw the focus only in text rectangle)
    void drawFocus(QPainter *painter
            , const QStyleOptionViewItem &option
            , const QRect &/*rect*/) const override
    {
        // don't override the rectangle to the text-only
        return QItemDelegate::drawFocus(painter, option, option.rect);
    }

    void drawDisplay(QPainter *painter
            , const QStyleOptionViewItem &option
            , const QRect &rect
            , const QString &text) const override
    {
        // shrink (and move to bottom) the text rectangle
        QRect r = rect.adjusted(0, MARGINS.top(), 0, 0);
        return QItemDelegate::drawDisplay(painter, option, r, text);
    }

    void drawDecoration(QPainter *painter
            , const QStyleOptionViewItem &option
            , const QRect &rect
            , const QPixmap &pixmap) const override
    {
        // move to bottom the pixmap rectangle
        QRect r = rect.translated(0, MARGINS.top());
        return QItemDelegate::drawDecoration(painter, option, r, pixmap);
    }
private:
    mutable QSize mItemSize; //!< the cached (unified/max) item size
};
constexpr QMargins ItemDelegate::MARGINS;

ListWidget::ListWidget(QWidget * parent/* = nullptr*/)
    : QListWidget{parent}
    , mRows(3)
    , mColumns(3)
{
    ItemDelegate * delegate = new ItemDelegate{this};
    {
        QScopedPointer<QAbstractItemDelegate> old_del{itemDelegate()};
        setItemDelegate(delegate);
    }
}

void ListWidget::setRows(int rows)
{
    mRows = rows;
}

void ListWidget::setColumns(int columns)
{
    mColumns = columns;
}

QSize ListWidget::viewportSizeHint() const
{
    QSize size = sizeHintForIndex(model()->index(0, 0));
    size.rwidth() = size.width() * mColumns + spacing() * mColumns * 2 + 1;
    size.rheight() = size.height() * mRows + spacing() * mRows * 2 + 1;
    return size;
}

QModelIndex ListWidget::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers/* modifiers*/)
{
    QModelIndex index = currentIndex();
    int count = model()->rowCount(rootIndex());
    int current = 0;
    if (index.isValid())
    {
        current = index.row();
        index = QModelIndex{}; // setting to invalid to get inside the for loop (move the position)
    } else
    {
        current = cursorAction == MovePrevious ? count - 1 : 0;
        index = model()->index(current, 0, rootIndex());
    }

    // if not enabled, try to find any next enabled
    for (int tries = 1; tries < count && 0 == (model()->flags(index) & Qt::ItemIsEnabled); ++tries)
    {
        int next;
        switch (cursorAction)
        {
            case MoveUp:
            case MovePageUp:
                next = (current - mColumns) % count;
                break;
            case MoveDown:
            case MovePageDown:
                next = (current + mColumns) % count;
                break;
            case MoveLeft:
                if (0 == (current % mColumns))
                    current += mColumns;
                // fall through
            case MovePrevious:
                if (current == 0)
                    return QModelIndex{};
                next = (current - 1) % count;
                break;
            case MoveRight:
                if ((mColumns - 1) == (current % mColumns))
                    current -= mColumns;
                // fall through
            case MoveNext:
                if (current == count - 1)
                    return QModelIndex{};
                next = (current + 1) % count;
                break;
            case MoveHome:
                next = 0;
                break;
            case MoveEnd:
                next = count - 1;
                break;
        }
        if (next < 0)
            next += count;

        index = model()->index(next, 0, rootIndex());
        current = next;
    }
    return index;

}

void ListWidget::keyPressEvent(QKeyEvent * event)
{
    if (event->key() == Qt::Key_Space)
    {
        // mimic the "enter" to fire activated
        QKeyEvent k{event->type(), Qt::Key_Enter, event->modifiers(), event->text(), event->isAutoRepeat(), static_cast<ushort>(event->count())};
        QListWidget::keyPressEvent(&k);
        event->setAccepted(k.isAccepted());
        return;
    }
    return QListWidget::keyPressEvent(event);
}

void ListWidget::focusInEvent(QFocusEvent * event)
{
    switch (event->reason())
    {
        case Qt::TabFocusReason:
            setCurrentIndex(model()->index(0, 0, rootIndex()));
            break;
        case Qt::BacktabFocusReason:
            setCurrentIndex(model()->index(model()->rowCount(rootIndex()) - 1, 0, rootIndex()));
            break;
        default:
            break;
    }
    return QListWidget::focusInEvent(event);
}
