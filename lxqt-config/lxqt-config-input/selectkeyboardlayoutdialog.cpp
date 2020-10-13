/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
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

#include "selectkeyboardlayoutdialog.h"
#include <QDebug>
#include <QPushButton>
SelectKeyboardLayoutDialog::SelectKeyboardLayoutDialog(QMap< QString, KeyboardLayoutInfo>& knownLayouts, QWidget* parent):
    QDialog(parent),
    knownLayouts_(knownLayouts) {
    ui.setupUi(this);
    ui.buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui.buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    connect(ui.layouts, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), SLOT(onLayoutChanged()));
    QMap<QString, KeyboardLayoutInfo >::const_iterator it;
    for(it = knownLayouts_.constBegin(); it != knownLayouts_.constEnd(); ++it) {
        const QString& name = it.key();
        const KeyboardLayoutInfo& info = *it;
        QListWidgetItem * item = new QListWidgetItem(info.description);
        item->setData(Qt::UserRole, name);
        ui.layouts->addItem(item);
    }
    ui.layouts->setCurrentItem(ui.layouts->item(0));
}
SelectKeyboardLayoutDialog::~SelectKeyboardLayoutDialog() {

}

void SelectKeyboardLayoutDialog::onLayoutChanged() {
    QListWidgetItem* item = ui.layouts->currentItem();
    ui.variants->clear();

    ui.variants->addItem(QStringLiteral("None"));
    ui.variants->setCurrentItem(ui.variants->item(0));
    if(item) { // add variants of this layout to the list view
        QString name = item->data(Qt::UserRole).toString();
        const KeyboardLayoutInfo& info = knownLayouts_[name];
        for(const LayoutVariantInfo& vinfo : qAsConst(info.variants)) {
            QListWidgetItem * vitem = new QListWidgetItem(vinfo.description);
            // qDebug() << "vitem" << vinfo.name << vinfo.description;
            vitem->setData(Qt::UserRole, vinfo.name);
            ui.variants->addItem(vitem);
        }
    }
}

QString SelectKeyboardLayoutDialog::selectedLayout() {
    QListWidgetItem* layoutItem = ui.layouts->currentItem();
    if(layoutItem) {
        return layoutItem->data(Qt::UserRole).toString();
    }
    return QString();
}

QString SelectKeyboardLayoutDialog::selectedVariant() {
    QListWidgetItem* variantItem = ui.variants->currentItem();
    if(variantItem)
        return variantItem->data(Qt::UserRole).toString();
    return QString();
}
