/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */


#include "lxqtpageselectwidget.h"
#include <QDebug>
#include <QStyledItemDelegate>
#include <QEvent>
#include <QScrollBar>
#include <QApplication>

using namespace LXQt;

class PageSelectWidgetItemDelegate: public QStyledItemDelegate
{
public:
    explicit PageSelectWidgetItemDelegate(PageSelectWidget *parent = nullptr);
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    PageSelectWidget* mView;
};


/************************************************

 ************************************************/
PageSelectWidgetItemDelegate::PageSelectWidgetItemDelegate(PageSelectWidget *parent):
    QStyledItemDelegate(parent),
    mView(parent)
{
}


/************************************************

 ************************************************/
QSize PageSelectWidgetItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);

    //all items should have unified width

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QWidget *widget = option.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    QSize size = style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), widget);
    //NOTE: this margin logic follows code in QCommonStylePrivate::viewItemLayout()
    const int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option, option.widget) + 1;

    // Find the extra vertical gap by subtracting the height of the text when wrapped within
    // the default size from the height of the text when wrapped within our max. width.
    const QRect R1 = mView->fontMetrics().boundingRect(QRect(0, 0, size.width() - 2 * margin, 0),
                                                        Qt::AlignLeft | Qt::TextWordWrap, opt.text);
    const QRect R2 = mView->fontMetrics().boundingRect(QRect(0, 0, mView->getWrappedTextWidth(), 0),
                                                        Qt::AlignLeft | Qt::TextWordWrap, opt.text);
    // compensate for the vertical gap
    size.rheight() -= qAbs(R1.height() - R2.height());

    //considering the icon size too
    size.setWidth(qMax(mView->maxTextWidth(), option.decorationSize.width()));
    // adding the margin is good but not necessary because it is already added
    size.rwidth() += 2 * margin;
    size.rheight() += margin; // only at the bottom

    return size;
}



/************************************************

 ************************************************/
PageSelectWidget::PageSelectWidget(QWidget *parent) :
    QListWidget(parent)
    , mMaxTextWidth(0)
{
    mWrappedTextWidth = fontMetrics().averageCharWidth() * 13; // max. 13 characters

    setSelectionRectVisible(false);
    setViewMode(IconMode);
    setSpacing(2);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    setWordWrap(true);
    setDragEnabled(NoDragDrop);
    setEditTriggers(NoEditTriggers);
    setTextElideMode(Qt::ElideNone);
    setContentsMargins(0, 0, 0, 0);

    setItemDelegate(new PageSelectWidgetItemDelegate(this));
    connect(model(), &QAbstractItemModel::rowsInserted, this, &PageSelectWidget::updateMaxTextWidth);
    connect(model(), &QAbstractItemModel::rowsRemoved, this, &PageSelectWidget::updateMaxTextWidth);
    connect(model(), &QAbstractItemModel::dataChanged, this, &PageSelectWidget::updateMaxTextWidth);
}


/************************************************

 ************************************************/
PageSelectWidget::~PageSelectWidget()
{
}

/************************************************

 ************************************************/
int PageSelectWidget::maxTextWidth() const
{
    return mMaxTextWidth;
}

/************************************************

 ************************************************/
QSize PageSelectWidget::viewportSizeHint() const
{
    const int spacing2 = spacing() * 2;
    QSize size{sizeHintForColumn(0) + spacing2, (sizeHintForRow(0) + spacing2) * count()};
    auto vScrollbar = verticalScrollBar();
    if (vScrollbar
        && vScrollbar->isVisible()
        && !vScrollbar->style()->styleHint(QStyle::SH_ScrollBar_Transient, nullptr, vScrollbar))
    {
        size.rwidth() += verticalScrollBar()->sizeHint().width();
    }
    return size;
}

/************************************************

 ************************************************/
QSize PageSelectWidget::sizeHint() const
{
    const int f = 2 * frameWidth();
    return viewportSizeHint() + QSize(f, f);
}

/************************************************

 ************************************************/
QSize PageSelectWidget::minimumSizeHint() const
{
    return QSize{0, 0};
}

/************************************************

 ************************************************/
void PageSelectWidget::updateMaxTextWidth()
{
    for(int i = count() - 1; 0 <= i; --i)
    {
        const QRect r = fontMetrics().boundingRect(QRect(0, 0, mWrappedTextWidth, 0),
                                                    Qt::AlignLeft | Qt::TextWordWrap, item(i)->text());
        mMaxTextWidth = qMax(mMaxTextWidth, r.width());
    }
}

/************************************************

 ************************************************/
bool PageSelectWidget::event(QEvent * event)
{
    if (QEvent::StyleChange == event->type())
        updateMaxTextWidth();

    return QListWidget::event(event);
}
