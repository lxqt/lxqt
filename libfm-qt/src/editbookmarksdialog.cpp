/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#include "editbookmarksdialog.h"
#include "ui_edit-bookmarks.h"
#include <QByteArray>
#include <QUrl>
#include <QSaveFile>
#include <QPushButton>
namespace Fm {

EditBookmarksDialog::EditBookmarksDialog(std::shared_ptr<Bookmarks> bookmarks, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    ui(new Ui::EditBookmarksDialog()),
    bookmarks_(std::move(bookmarks)) {
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    setAttribute(Qt::WA_DeleteOnClose); // auto delete on close
    // load bookmarks
    for(const auto& bookmark: bookmarks_->items()) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setData(0, Qt::DisplayRole, bookmark->name());
        item->setData(1, Qt::DisplayRole, QString::fromUtf8(bookmark->path().toString().get()));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
        ui->treeWidget->addTopLevelItem(item);
    }

    connect(ui->addItem, &QPushButton::clicked, this, &EditBookmarksDialog::onAddItem);
    connect(ui->removeItem, &QPushButton::clicked, this, &EditBookmarksDialog::onRemoveItem);
}

EditBookmarksDialog::~EditBookmarksDialog() {
    delete ui;
}

void EditBookmarksDialog::accept() {
    // save bookmarks
    // it's easier to recreate the whole bookmark file than
    // to manipulate Fm::Bookmarks object. So here we generate the file directly.
    // FIXME: maybe in the future we should add a libfm-qt API to easily replace all Fm::Bookmarks.
    QString path = QString::fromUtf8(bookmarks_->bookmarksFile().toString().get());
    QSaveFile file(path); // use QSaveFile for atomic file operation
    if(file.open(QIODevice::WriteOnly)) {
        for(int row = 0; ; ++row) {
            QTreeWidgetItem* item = ui->treeWidget->topLevelItem(row);
            if(!item) {
                break;
            }
            QString name = item->data(0, Qt::DisplayRole).toString();
            QUrl url = QUrl::fromUserInput(item->data(1, Qt::DisplayRole).toString());
            if (!url.isValid())
                url = QUrl::fromUserInput(QString::fromUtf8(Fm::FilePath::homeDir().toString().get()));
            file.write(url.toEncoded());
            file.write(" ");
            file.write(name.toUtf8());
            file.write("\n");
        }
        // FIXME: should we support Qt or KDE specific bookmarks in the future?
        file.commit();
    }
    QDialog::accept();
}

void EditBookmarksDialog::onAddItem() {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setData(0, Qt::DisplayRole, tr("New bookmark"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled);
    ui->treeWidget->addTopLevelItem(item);
    ui->treeWidget->editItem(item);
}

void EditBookmarksDialog::onRemoveItem() {
    const QList<QTreeWidgetItem*> sels = ui->treeWidget->selectedItems();
    for(QTreeWidgetItem* const item : sels) {
        delete item;
    }
}


} // namespace Fm
