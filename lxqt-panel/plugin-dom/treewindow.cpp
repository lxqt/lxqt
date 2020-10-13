/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
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


#include "treewindow.h"
#include "ui_treewindow.h"
#include "domtreeitem.h"
#include <QDebug>
#include <QTableWidget>
#include <QMetaProperty>

#define PROP_OBJECT_NAME    0
#define PROP_CLASS_NAME     1
#define PROP_TEXT           2
#define PROP_CLASS_HIERARCY 3


TreeWindow::TreeWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TreeWindow)
{
    mRootWidget = this;
    while (mRootWidget->parentWidget())
        mRootWidget = mRootWidget->parentWidget();

    ui->setupUi(this);

    ui->tree->setStyleSheet(QStringLiteral(
                "QTreeView::item { "
                    "padding: 2px;"
                "}"
                ));

    initPropertiesView();

    QList<QWidget*> widgets = mRootWidget->findChildren<QWidget*>();

    ui->tree->setRootIsDecorated(false);

    DomTreeItem *item = new DomTreeItem(ui->tree, mRootWidget);
    ui->tree->expandItem(item);
    ui->tree->resizeColumnToContents(0);


    connect(ui->tree, SIGNAL(itemSelectionChanged()), this, SLOT(updatePropertiesView()));
    item->setSelected(true);

    QHeaderView* h = new QHeaderView(Qt::Horizontal);
    h->setStretchLastSection(true);
    ui->allPropertiesView->setHorizontalHeader(h);
    connect(h, &QHeaderView::sectionDoubleClicked, this, &TreeWindow::sectionDoubleClickedSlot);
}


TreeWindow::~TreeWindow()
{
    delete ui;
}


void TreeWindow::initPropertiesView()
{
    ui->propertiesView->viewport()->setAutoFillBackground(false);

    ui->propertiesView->setRowCount(4);
    ui->propertiesView->setColumnCount(2);

    QTableWidgetItem *item;
    item = new  QTableWidgetItem(QStringLiteral("Object name"));
    ui->propertiesView->setItem(PROP_OBJECT_NAME, 0, item);
    ui->propertiesView->setItem(PROP_OBJECT_NAME, 1, new QTableWidgetItem());

    item = new  QTableWidgetItem(QStringLiteral("Class name"));
    ui->propertiesView->setItem(PROP_CLASS_NAME, 0, item);
    ui->propertiesView->setItem(PROP_CLASS_NAME, 1, new QTableWidgetItem());

    item = new  QTableWidgetItem(QStringLiteral("Text"));
    ui->propertiesView->setItem(PROP_TEXT, 0, item);
    ui->propertiesView->setItem(PROP_TEXT, 1, new QTableWidgetItem());

    item = new  QTableWidgetItem(QStringLiteral("Class hierarcy"));
    ui->propertiesView->setItem(PROP_CLASS_HIERARCY, 0, item);
    ui->propertiesView->setItem(PROP_CLASS_HIERARCY, 1, new QTableWidgetItem());


}


void TreeWindow::updatePropertiesView()
{
    if (ui->tree->selectedItems().isEmpty())
    {
        clearPropertiesView();
        return;
    }

    QTreeWidgetItem *item = ui->tree->selectedItems().first();
    if (!item)
    {
        clearPropertiesView();
        return;
    }

    DomTreeItem *treeItem = static_cast<DomTreeItem*>(item);

    ui->propertiesView->item(PROP_OBJECT_NAME, 1)->setText(treeItem->widgetObjectName());
    ui->propertiesView->item(PROP_CLASS_NAME, 1)->setText(treeItem->widgetClassName());
    ui->propertiesView->item(PROP_TEXT, 1)->setText(treeItem->widgetText());
    ui->propertiesView->item(PROP_CLASS_HIERARCY, 1)->setText(treeItem->widgetClassHierarcy().join(QStringLiteral(" :: ")));

    QString s;
    QDebug out(&s);
    QMetaObject const * const m = treeItem->widget()->metaObject();
    const int curr_cnt = ui->allPropertiesView->rowCount();
    ui->allPropertiesView->setRowCount(m->propertyCount());
    for (int i = 0, cnt = m->propertyCount(); cnt > i; ++i)
    {
        if (curr_cnt <= i)
        {
            ui->allPropertiesView->setItem(i, 0, new QTableWidgetItem);
            ui->allPropertiesView->setItem(i, 1, new QTableWidgetItem);
            ui->allPropertiesView->setItem(i, 2, new QTableWidgetItem);
        }
        QMetaProperty const & prop = m->property(i);
        ui->allPropertiesView->item(i, 0)->setText(QString::fromUtf8(prop.name()));
        ui->allPropertiesView->item(i, 1)->setText(QString::fromUtf8(prop.typeName()));
        s.clear();
        out << prop.read(treeItem->widget());
        ui->allPropertiesView->item(i, 2)->setText(s);
    }
    for (int i = m->propertyCount(); curr_cnt > i; ++i)
        ui->allPropertiesView->removeRow(i);
}


void TreeWindow::clearPropertiesView()
{
    for (int i=0; i<ui->propertiesView->rowCount(); ++i)
        ui->propertiesView->item(i, 1)->setText(QLatin1String(""));
    for (int i = ui->allPropertiesView->rowCount(); 0 <=  i; --i)
        ui->allPropertiesView->removeRow(i);
    ui->allPropertiesView->setRowCount(0);
}

void TreeWindow::sectionDoubleClickedSlot(int column)
{
    ui->allPropertiesView->sortByColumn(column, Qt::AscendingOrder);
}
