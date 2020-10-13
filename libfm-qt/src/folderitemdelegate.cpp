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


#include "folderitemdelegate.h"
#include "foldermodel.h"
#include <QPainter>
#include <QModelIndex>
#include <QAbstractItemView>
#include <QStyleOptionViewItem>
#include <QApplication>
#include <QIcon>
#include <QTextLayout>
#include <QTextOption>
#include <QTextLine>
#include <QLineEdit>
#include <QTextEdit>
#include <QTimer>
#include <QStandardPaths>
#include <QDebug>

namespace Fm {

FolderItemDelegate::FolderItemDelegate(QAbstractItemView* view, QObject* parent):
    QStyledItemDelegate(parent ? parent : view),
    symlinkIcon_(QIcon::fromTheme(QStringLiteral("emblem-symbolic-link"))),
    untrustedIcon_(QIcon::fromTheme(QStringLiteral("emblem-important"))),
    addIcon_(QIcon::fromTheme(QStringLiteral("list-add"))),
    removeIcon_(QIcon::fromTheme(QStringLiteral("list-remove"))),
    fileInfoRole_(Fm::FolderModel::FileInfoRole),
    iconInfoRole_(-1),
    margins_(QSize(3, 3)),
    shadowHidden_(false),
    hasEditor_(false) {
    connect(this,  &QAbstractItemDelegate::closeEditor, [=]{hasEditor_ = false;});
}

FolderItemDelegate::~FolderItemDelegate() {

}

QSize FolderItemDelegate::iconViewTextSize(const QModelIndex& index) const {
    QStyleOptionViewItem opt;
    initStyleOption(&opt, index);
    opt.decorationSize = iconSize_.isValid() ? iconSize_ : QSize(0, 0);
    opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignTop;
    opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;
    QRectF textRect(0, 0,
                    itemSize_.width() - 2 * margins_.width(),
                    itemSize_.height() - 2 * margins_.height() - opt.decorationSize.height());
    drawText(nullptr, opt, textRect); // passing nullptr for painter will calculate the bounding rect only
    return textRect.toRect().size();
}

QSize FolderItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QVariant value = index.data(Qt::SizeHintRole);
    if(value.isValid()) {
        // no further processing if the size is specified by the data model
        return qvariant_cast<QSize>(value);
    }

    if(option.decorationPosition == QStyleOptionViewItem::Top ||
            option.decorationPosition == QStyleOptionViewItem::Bottom) {
        // we handle vertical layout just by returning our item size
        return itemSize_;
    }

    // The default size hint of the horizontal layout isn't reliable
    // because Qt calculates the row size based on the real icon size,
    // which may not be equal to the requested icon size on various occasions.
    // So, we do as in QStyledItemDelegate::sizeHint() but use the requested size.
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    opt.decorationSize = option.decorationSize; // requested by the view
    const QWidget* widget = option.widget;
    QStyle* style = widget ? widget->style() : QApplication::style();
    return style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), widget);
}

QIcon::Mode FolderItemDelegate::iconModeFromState(const QStyle::State state) {

    if(state & QStyle::State_Enabled) {
        return (state & QStyle::State_Selected) ? QIcon::Selected : QIcon::Normal;
    }

    return QIcon::Disabled;
}

void FolderItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if(!index.isValid())
        return;

    // get emblems for this icon
    std::forward_list<std::shared_ptr<const Fm::IconInfo>> icon_emblems;
    auto fmicon = index.data(iconInfoRole_).value<std::shared_ptr<const Fm::IconInfo>>();
    if(fmicon) {
        icon_emblems = fmicon->emblems();
    }
    // get file info for the item
    auto file = index.data(fileInfoRole_).value<std::shared_ptr<const Fm::FileInfo>>();
    const auto& emblems = file ? file->emblems() : icon_emblems;

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // distinguish the hidden items visually by making their texts italic
    bool shadowIcon(false);
    if(file && file->isHidden()) {
        QFont f(opt.font);
        f.setItalic(true);
        opt.font = f;
        shadowIcon = shadowHidden_;
    }

    bool isSymlink = file && file->isSymlink();
    bool isCut = index.data(FolderModel::FileIsCutRole).toBool();
    // an emblem is added only to an untrusted, deletable desktop file that isn't inside applications directory
    bool untrusted = file && !file->isTrustable() && file->isDesktopEntry() && file->isDeletable();
    if(untrusted) {
        auto parentDir = QString::fromUtf8((file->dirPath().toString().get()));
        if(QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation).contains(parentDir)) {
            untrusted = false;
        }
    }
    // vertical layout (icon mode, thumbnail mode)
    if(option.decorationPosition == QStyleOptionViewItem::Top ||
            option.decorationPosition == QStyleOptionViewItem::Bottom) {
        painter->save();
        painter->setClipRect(option.rect);

        opt.decorationAlignment = Qt::AlignHCenter | Qt::AlignTop;
        opt.displayAlignment = Qt::AlignTop | Qt::AlignHCenter;

        // draw the icon
        QIcon::Mode iconMode = shadowIcon ? QIcon::Disabled
                                          // in the icon and thumbnail modes, we select text, not icon
                                          : iconModeFromState(opt.state & ~QStyle::State_Selected);
        QPoint iconPos(opt.rect.x() + (opt.rect.width() - option.decorationSize.width()) / 2, opt.rect.y() + margins_.height());
        QRect iconRect(iconPos, option.decorationSize);
        if(isCut) {
            painter->save();
            painter->setOpacity(0.45);
        }
        opt.icon.paint(painter, iconRect, Qt::AlignCenter, iconMode);
        if(isCut) {
            painter->restore();
        }

        // draw some emblems for the item if needed
        iconRect.setSize(option.decorationSize / 2);
        if(isSymlink) {
            // draw the emblem for symlinks
            symlinkIcon_.paint(painter, iconRect, Qt::AlignCenter, iconMode);
        }

        if(untrusted) {
            // emblem for untrusted, deletable desktop files
            untrustedIcon_.paint(painter, iconRect.translated(0, option.decorationSize.height() / 2), Qt::AlignCenter, iconMode);
        }

        // draw other emblems if there's any
        if(!emblems.empty()) {
            // FIXME: we only support one emblem now
            QIcon emblem = emblems.front()->qicon();
            emblem.paint(painter, iconRect.translated(option.decorationSize.width() / 2, option.decorationSize.height() / 2), Qt::AlignCenter, iconMode);
        }

        // Draw select/deselect icons outside the main icon but near its top left corner,
        // with its 1/3 size and only if the icon size isn't smaller than 48 px
        // (otherwise, the user could not click on them easily).
        const QAbstractItemView* iv = qobject_cast<const QAbstractItemView*>(opt.widget);
        if(iv != nullptr
           // only for the extended and multiple selection modes
           && (iv->selectionMode() == QAbstractItemView::ExtendedSelection
               || iv->selectionMode() == QAbstractItemView::MultiSelection)
           && option.decorationSize.width() >= 48 && (opt.state & QStyle::State_MouseOver)) {
            int s = option.decorationSize.width() / 3;
            bool cursorOnSelectionCorner = false;
            iconPos = QPoint(qMax(opt.rect.x(), iconPos.x() - s),
                             qMax(opt.rect.y(), iconPos.y() - s));
            QPoint curPos = iv->viewport()->mapFromGlobal(QCursor::pos());
            if(curPos.x() >= iconPos.x() && curPos.x() <= iconPos.x() + s
               && curPos.y() >= iconPos.y() && curPos.y() <= iconPos.y() + s) {
                cursorOnSelectionCorner = true;
            }
            if(!cursorOnSelectionCorner) { // make it translucent when not under the cursor
                painter->save();
                painter->setOpacity(0.6);
            }
            iconRect = QRect(iconPos, QSize(s, s));
            if(opt.state & QStyle::State_Selected) {
                removeIcon_.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal);
            }
            else {
                addIcon_.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal);
            }
            if(!cursorOnSelectionCorner) {
                painter->restore();
            }
        }

        // draw the text
        QSize drawAreaSize = itemSize_ - 2 * margins_;
        // The text rect dimensions should be exactly as they were in sizeHint()
        QRectF textRect(opt.rect.x() + (opt.rect.width() - drawAreaSize.width()) / 2,
                        opt.rect.y() + margins_.height() + option.decorationSize.height(),
                        drawAreaSize.width(),
                        drawAreaSize.height() - option.decorationSize.height());
        drawText(painter, opt, textRect);
        painter->restore();
    }
    else {  // horizontal layout (list view)

        // Let the style engine do the painting but take care of shadowed and cut icons.
        // NOTE: The shadowing can also be done directly.
        // WARNING: QStyledItemDelegate shouldn't be used for painting because it resets the icon.
        // FIXME: For better text alignment, here we could have increased the icon width
        // when it's smaller than the requested size.

        QIcon::Mode iconMode = shadowIcon ? QIcon::Disabled : iconModeFromState(opt.state);

        if(!opt.icon.isNull()) {
            if(shadowIcon) {
                QPixmap pixmap = opt.icon.pixmap(option.decorationSize, iconMode);
                opt.icon = QIcon(pixmap);
            }

            if(isCut) {
                QPixmap pixmap = opt.icon.pixmap(option.decorationSize, iconMode);
                QImage img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
                img.fill(Qt::transparent);
                QPainter p(&img);
                p.setOpacity(0.45);
                p.drawPixmap(0, 0, pixmap);
                p.end();
                opt.icon = QIcon(QPixmap::fromImage(img));
            }
        }

        const QWidget* widget = opt.widget;
        QStyle* style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

        if(isCut && !opt.icon.isNull()) {
            // restore the original icon to not make it translucent more than once
            opt.icon = option.icon;
        }

        // draw some emblems for the item if needed
        if(isSymlink) {
            QPoint iconPos(opt.rect.x(), opt.rect.y() + (opt.rect.height() - option.decorationSize.height()) / 2);
            QRect iconRect(iconPos, option.decorationSize / 2);
            symlinkIcon_.paint(painter, iconRect, Qt::AlignCenter, iconMode);
        }
        if(untrusted) {
            QPoint iconPos(opt.rect.x(), opt.rect.y() + opt.rect.height() / 2);
            QRect iconRect(iconPos, option.decorationSize / 2);
            untrustedIcon_.paint(painter, iconRect, Qt::AlignCenter, iconMode);
        }
        if(!emblems.empty()) {
            // FIXME: we only support one emblem now
            QPoint iconPos(opt.rect.x() + option.decorationSize.width() / 2, opt.rect.y() + opt.rect.height() / 2);
            QRect iconRect(iconPos, option.decorationSize / 2);
            QIcon emblem = emblems.front()->qicon();
            emblem.paint(painter, iconRect, Qt::AlignCenter, iconMode);
        }
    }
}

// if painter is nullptr, the method calculate the bounding rectangle of the text and save it to textRect
void FolderItemDelegate::drawText(QPainter* painter, QStyleOptionViewItem& opt, QRectF& textRect) const {
    QTextLayout layout(opt.text, opt.font);
    QTextOption textOption;
    textOption.setAlignment(opt.displayAlignment);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    // FIXME:     textOption.setTextDirection(opt.direction); ?
    if(opt.text.isRightToLeft()) {
        textOption.setTextDirection(Qt::RightToLeft);
    }
    else {
        textOption.setTextDirection(Qt::LeftToRight);
    }
    layout.setTextOption(textOption);
    qreal height = 0;
    qreal width = 0;
    int visibleLines = 0;
    layout.beginLayout();
    QString elidedText;
    textRect.adjust(2, 2, -2, -2); // a 2-px margin is considered at FolderView::updateGridSize()
    for(;;) {
        QTextLine line = layout.createLine();
        if(!line.isValid()) {
            break;
        }
        line.setLineWidth(textRect.width());
        height += opt.fontMetrics.leading();
        line.setPosition(QPointF(0, height));
        if((height + line.height() + textRect.y()) > textRect.bottom()) {
            // if part of this line falls outside the textRect, ignore it and quit.
            QTextLine lastLine = layout.lineAt(visibleLines - 1);
            elidedText = opt.text.mid(lastLine.textStart());
            elidedText = opt.fontMetrics.elidedText(elidedText, opt.textElideMode, textRect.width());
            if(visibleLines == 1) { // this is the only visible line
                width = textRect.width();
            }
            break;
        }
        height += line.height();
        width = qMax(width, line.naturalTextWidth());
        ++ visibleLines;
    }
    layout.endLayout();

#if (QT_VERSION >= QT_VERSION_CHECK(5,11,0))
    width = qMax(width, static_cast<qreal>(opt.fontMetrics.horizontalAdvance(elidedText)));
#else
    width = qMax(width, (qreal)opt.fontMetrics.width(elidedText));
#endif

    // draw background for selected item
    QRectF boundRect = layout.boundingRect();
    //qDebug() << "bound rect: " << boundRect << "width: " << width;
    boundRect.setWidth(width);
    boundRect.setHeight(height);
    boundRect.moveTo(textRect.x() + (textRect.width() - width) / 2, textRect.y());

    QRectF selRect = boundRect.adjusted(-2, -2, 2, 2);

    if(!painter) { // no painter, calculate the bounding rect only
        textRect = selRect;
        return;
    }

    // Respect the active and inactive palettes (some styles can use different colors for them).
    // Also, take into account a probable disabled palette.
    QPalette::ColorGroup cg = (opt.state & QStyle::State_Enabled)
                                  ? (opt.state & QStyle::State_Active)
                                      ? QPalette::Active
                                      : QPalette::Inactive
                                  : QPalette::Disabled;
    if(opt.state & QStyle::State_Selected) {
        if(!opt.widget) {
            painter->fillRect(selRect, opt.palette.highlight());
        }
        painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
    }
    else {
        painter->setPen(opt.palette.color(cg, QPalette::Text));
    }

    if(opt.state & QStyle::State_Selected || opt.state & QStyle::State_MouseOver) {
        if(const QWidget* widget = opt.widget) {  // let the style engine do it
            QStyle* style = widget->style() ? widget->style() : qApp->style();
            QStyleOptionViewItem o(opt);
            o.text = QString();
            o.rect = selRect.toAlignedRect().intersected(opt.rect); // due to clipping and rounding, we might lose 1px
            o.showDecorationSelected = true;
            style->drawPrimitive(QStyle::PE_PanelItemViewItem, &o, painter, widget);
        }
    }

    // draw shadow for text if the item is not selected and a shadow color is set
    if(!(opt.state & QStyle::State_Selected) && shadowColor_.isValid()) {
        QPen prevPen = painter->pen();
        painter->setPen(QPen(shadowColor_));
        for(int i = 0; i < visibleLines; ++i) {
            QTextLine line = layout.lineAt(i);
            if(i == (visibleLines - 1) && !elidedText.isEmpty()) { // the last line, draw elided text
                QPointF pos(boundRect.x() + line.position().x() + 1, boundRect.y() + line.y() + line.ascent() + 1);
                painter->drawText(pos, elidedText);
            }
            else {
                line.draw(painter, textRect.topLeft() + QPointF(1, 1));
            }
        }
        painter->setPen(prevPen);
    }

    // draw text
    for(int i = 0; i < visibleLines; ++i) {
        QTextLine line = layout.lineAt(i);
        if(i == (visibleLines - 1) && !elidedText.isEmpty()) { // the last line, draw elided text
            QPointF pos(boundRect.x() + line.position().x(), boundRect.y() + line.y() + line.ascent());
            painter->drawText(pos, elidedText);
        }
        else {
            line.draw(painter, textRect.topLeft());
        }
    }

    if(opt.state & QStyle::State_HasFocus) {
        // draw focus rect
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(opt);
        o.rect = selRect.toRect(); // subElementRect(SE_ItemViewItemFocusRect, vopt, widget);
        o.state |= QStyle::State_KeyboardFocusChange;
        o.state |= QStyle::State_Item;
        QPalette::ColorGroup cg = (opt.state & QStyle::State_Enabled)
                                  ? QPalette::Normal : QPalette::Disabled;
        o.backgroundColor = opt.palette.color(cg, (opt.state & QStyle::State_Selected)
                                              ? QPalette::Highlight : QPalette::Window);
        if(const QWidget* widget = opt.widget) {
            QStyle* style = widget->style() ? widget->style() : qApp->style();
            style->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter, widget);
        }
    }
}

/*
 * The following methods are for inline renaming.
 */

QWidget* FolderItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    hasEditor_ = true;
    if (option.decorationPosition == QStyleOptionViewItem::Top
        || option.decorationPosition == QStyleOptionViewItem::Bottom)
    {
        // in icon view, we use QTextEdit as the editor (and not QPlainTextEdit
        // because the latter always shows an empty space at the bottom)
        QTextEdit *textEdit = new QTextEdit(parent);
        textEdit->setAcceptRichText(false);

        // Since the text color on desktop is inherited from desktop foreground color,
        // it may not be suitable. So, we reset it by using the app palette.
        QPalette p = textEdit->palette();
        p.setColor(QPalette::Text, qApp->palette().text().color());
        textEdit->setPalette(p);

        textEdit->ensureCursorVisible();
        textEdit->setFocusPolicy(Qt::StrongFocus);
        textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        textEdit->setContentsMargins(0, 0, 0, 0);
        return textEdit;
    }
    else {
        // return the default line-edit in other views and
        // ensure that its background isn't transparent (on the side-pane)
        QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
        QPalette p = editor->palette();
        p.setColor(QPalette::Text, qApp->palette().text().color());
        p.setColor(QPalette::Base, qApp->palette().color(QPalette::Base));
        editor->setPalette(p);
        return editor;
    }
}

void FolderItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }
    const QString currentName = index.data(Qt::EditRole).toString();

    if (QTextEdit* textEdit = qobject_cast<QTextEdit*>(editor)) {
        textEdit->setPlainText(currentName);
        textEdit->setUndoRedoEnabled(false);
        textEdit->setAlignment(Qt::AlignCenter);
        textEdit->setUndoRedoEnabled(true);
        // select text appropriately
        QTextCursor cur = textEdit->textCursor();
        int end;
        if (index.data(Fm::FolderModel::FileIsDirRole).toBool() || !currentName.contains(QLatin1String("."))) {
            end = currentName.size();
        }
        else {
            end = currentName.lastIndexOf(QLatin1String("."));
        }
        cur.setPosition(end, QTextCursor::KeepAnchor);
        textEdit->setTextCursor(cur);
    }
    else if (QLineEdit* lineEdit = qobject_cast<QLineEdit*>(editor)) {
        lineEdit->setText(currentName);
        if (!index.data(Fm::FolderModel::FileIsDirRole).toBool() && currentName.contains(QLatin1String(".")))
        {
            /* Qt will call QLineEdit::selectAll() after calling setEditorData() in
               qabstractitemview.cpp -> QAbstractItemViewPrivate::editor(). Therefore,
               we cannot select a part of the text in the usual way here.  */
            QTimer::singleShot(0, lineEdit, [lineEdit]() {
                int length = lineEdit->text().lastIndexOf(QLatin1String("."));
                lineEdit->setSelection(0, length);
            });
        }
    }
}

bool FolderItemDelegate::eventFilter(QObject* object, QEvent* event) {
    QWidget *editor = qobject_cast<QWidget*>(object);
    if (editor && event->type() == QEvent::KeyPress) {
        int k = static_cast<QKeyEvent *>(event)->key();
        if (k == Qt::Key_Return || k == Qt::Key_Enter) {
            Q_EMIT QAbstractItemDelegate::commitData(editor);
            Q_EMIT QAbstractItemDelegate::closeEditor(editor, QAbstractItemDelegate::NoHint);
            return true;
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}

void FolderItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (option.decorationPosition == QStyleOptionViewItem::Top
        || option.decorationPosition == QStyleOptionViewItem::Bottom) {
        // give all of the available space to the editor
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        opt.decorationAlignment = Qt::AlignHCenter|Qt::AlignTop;
        opt.displayAlignment = Qt::AlignTop|Qt::AlignHCenter;
        QRect textRect(opt.rect.x(),
                       opt.rect.y() + margins_.height() + option.decorationSize.height(),
                       itemSize_.width(),
                       itemSize_.height() - margins_.height() - option.decorationSize.height());
        int frame = editor->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, editor);
        editor->setGeometry(textRect.adjusted(-frame, -frame, frame, frame));
    }
    else {
        // use the default editor geometry in compact view
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}


} // namespace Fm
