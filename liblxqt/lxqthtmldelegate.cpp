/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Paulo Lieuthier <paulolieuthier@gmail.com>
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

#include "lxqthtmldelegate.h"
#include <QAbstractTextDocumentLayout>
#include <QTextDocument>
#include <QApplication>

using namespace LXQt;


HtmlDelegate::HtmlDelegate(const QSize iconSize, QObject* parent) :
    QStyledItemDelegate(parent),
    mIconSize(iconSize)
{
}

HtmlDelegate::~HtmlDelegate()
{
}

/************************************************

 ************************************************/
void HtmlDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (!index.isValid())
        return;

    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);
    const bool is_right_to_left = Qt::RightToLeft == options.direction;

    painter->save();

    QTextDocument doc;
    doc.setHtml(options.text);
    QIcon icon = options.icon;

    // icon size
    const QSize iconSize = icon.actualSize(mIconSize);
    // center the icon vertically
    QRect iconRect = QRect(8, qMax(options.rect.height() - iconSize.height(), 0) / 2,
                           iconSize.width(), iconSize.height());
    if (is_right_to_left)
    {
        iconRect.moveLeft(options.rect.left() + options.rect.right() - iconRect.x() - iconRect.width() + 1);
    }

    // set doc size
    // NOTE: Qt has a bug, because of which HTML tags are included in QTextDocument::setTextWidth()
    // when the text is set by QTextDocument::setHtml().
    // As a result, the text height may be a little greater than needed.
    doc.setTextWidth(options.rect.width() - iconRect.width() - 8 - 8); // 8-px icon-text spacing

    // draw the item's panel
    const QWidget* widget = option.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &options, painter, widget);

    // paint icon
    painter->translate(options.rect.left(), options.rect.top());
    icon.paint(painter, iconRect);

    // center the text vertically
    painter->translate(0, qMax(static_cast<qreal>(options.rect.height()) - doc.size().height(), 0.0) / 2.0);

    if (!is_right_to_left)
    {
        // shift text right to make icon visible
        painter->translate((iconRect.right() + 1) + 8, 0);
    }
    const QRect clip(0, 0, options.rect.width() - iconRect.width() - 8, options.rect.height());
    painter->setClipRect(clip);

    // set text colors
    QAbstractTextDocumentLayout::PaintContext ctx;
    QPalette::ColorGroup colorGroup = (option.state & QStyle::State_Active) ? QPalette::Active : QPalette::Inactive;
    if (option.state & QStyle::State_Selected) // selected items
        ctx.palette.setColor(QPalette::Text, option.palette.color(colorGroup, QPalette::HighlightedText));
    else // ordinary items and those with an alternate base color (there is no alternate text color)
        ctx.palette.setColor(QPalette::Text, option.palette.color(colorGroup, QPalette::Text));

    ctx.clip = clip;
    doc.documentLayout()->draw(painter, ctx);

    painter->restore();
}


/************************************************

 ************************************************/
QSize HtmlDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    const QSize iconSize = options.icon.actualSize(mIconSize);
    // 8-px left, top and bottom margins for the icon
    const QRect iconRect = QRect(8, 8, iconSize.width(), iconSize.height());
    const int w = options.rect.width();

    QTextDocument doc;
    doc.setHtml(options.text);

    if (w > 0)
        doc.setTextWidth(static_cast<qreal>(w - (iconRect.right() + 1) - 8)); // 8-px icon-text spacing
    else
        doc.adjustSize();
    return {w > 0 ? w : iconRect.width() + 8 + qRound(doc.size().width()) + 8,
                 qMax(qRound(doc.size().height() + 8), // 4-px top/bottom text spacing
                      iconSize.height() + 16)};
}
