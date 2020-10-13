/*
 * checkboxdelegate.cpp
 * This file is part of qps -- Qt-based visual process status monitor
 *
 * Copyright 1997-1999 Mattias Engdegård
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "checkboxdelegate.h"
#include <QSpinBox>

checkBoxDelegate::checkBoxDelegate(QObject *parent) : QItemDelegate(parent) {}
//! [0]

//! [1]
QWidget *checkBoxDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    printf("createEditor\n");
    // return 0;
    if (index.column() == 1)
    {
        QSpinBox *editor = new QSpinBox(parent);
        editor->setMinimum(0);
        editor->setMaximum(100);
        return editor;
    }
    return QItemDelegate::createEditor(parent, option, index);
    return nullptr;
}
//! [1]

//! [2]
void checkBoxDelegate::setEditorData(QWidget *editor,
                                     const QModelIndex &index) const
{
    printf("setEditorData\n");
    return;
    // if(index
    if (index.column() == 1)
    {
        int value = index.model()->data(index, Qt::EditRole).toInt();
        //	int value=0;
        QSpinBox *spinBox = static_cast<QSpinBox *>(editor);
        spinBox->setValue(value);
    }
}
//! [2]

//! [3]
void checkBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    printf("setModelData\n");
    return;
    if (index.column() == 1)
    {
        QSpinBox *spinBox = static_cast<QSpinBox *>(editor);
        spinBox->interpretText();
        int value = spinBox->value();
        model->setData(index, value, Qt::EditRole);
    }
}
//! [3]

//! [4]
void checkBoxDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex & /*index*/) const
{
    printf("updateEditorGeometry\n");
    //	if(index.column()==1)
    editor->setGeometry(option.rect);
}
//! [4]

void checkBoxDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    printf("paint\n");
    if (index.column() == 1)
    {
//        QVariant value = 0; // index.model()->data(index, Qt::UserRole);
                            /* if (!isSupportedType(value.type())) {
                                     QStyleOptionViewItem myOption = option;
                                     myOption.state &= ~QStyle::State_Enabled;
                                     QItemDelegate::paint(painter, myOption, index);
                                     return;
                             } */
    }
    QItemDelegate::paint(painter, option, index);
}
// QSize QAbstractItemDelegate::sizeHint ( const QStyleOptionViewItem & option,
// const QModelIndex & index ) const	 [pure virtual]
