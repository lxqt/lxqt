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


#include "renamedialog.h"
#include "ui_rename-dialog.h"
#include <QStringBuilder>
#include <QPushButton>
#include <QDateTime>

#include "core/iconinfo.h"
#include "utilities.h"

#include "core/legacy/fm-config.h"

namespace Fm {

RenameDialog::RenameDialog(const FileInfo &src, const FileInfo &dest, QWidget* parent, Qt::WindowFlags f):
    QDialog(parent, f),
    action_(ActionIgnore),
    applyToAll_(false) {
    ui = new Ui::RenameDialog();
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
    ui->buttonBox_2->button(QDialogButtonBox::Ignore)->setText(tr("Ignore"));
    ui->buttonBox_3->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    auto path = dest.path();
    auto srcIcon = src.icon();
    auto destIcon = dest.icon();
    // show info for the source file
    QIcon icon = srcIcon->qicon();
    // FIXME: deprecate fm_config
    QSize iconSize(fm_config->big_icon_size, fm_config->big_icon_size);
    QPixmap pixmap = icon.pixmap(iconSize);
    ui->srcIcon->setPixmap(pixmap);

    QString infoStr;
    // FIXME: deprecate fm_config
    auto disp_size = Fm::formatFileSize(src.size(), fm_config->si_unit);
    auto srcMtime = QDateTime::fromMSecsSinceEpoch(src.mtime() * 1000).toString(Qt::SystemLocaleShortDate);
    if(!disp_size.isEmpty()) {
        infoStr = QString(tr("Type: %1\nSize: %2\nModified: %3"))
                .arg(src.description(),
                     disp_size,
                     srcMtime);
    }
    else {
        infoStr = QString(tr("Type: %1\nModified: %2"))
                .arg(src.description(), srcMtime);
    }
    ui->srcInfo->setText(infoStr);

    // show info for the dest file
    icon = destIcon->qicon();
    pixmap = icon.pixmap(iconSize);
    ui->destIcon->setPixmap(pixmap);

    disp_size = Fm::formatFileSize(dest.size(), fm_config->si_unit);
    auto destMtime = QDateTime::fromMSecsSinceEpoch(dest.mtime() * 1000).toString(Qt::SystemLocaleShortDate);
    if(!disp_size.isEmpty()) {
        infoStr = QString(tr("Type: %1\nSize: %2\nModified: %3"))
                .arg(dest.description(),
                     disp_size,
                     destMtime);
    }
    else {
        infoStr = QString(tr("Type: %1\nModified: %2"))
                .arg(dest.description(),
                     destMtime);
    }
    ui->destInfo->setText(infoStr);

    auto basename = path.baseName();
    ui->fileName->setText(QString::fromUtf8(basename.get()));
    oldName_ = QString::fromUtf8(basename.get());
    connect(ui->fileName, &QLineEdit::textChanged, this, &RenameDialog::onFileNameChanged);

    // add "Rename" button
    QAbstractButton* button = ui->buttonBox_1->button(QDialogButtonBox::Ok);
    button->setText(tr("&Overwrite"));
    // FIXME: there seems to be no way to place the Rename button next to the overwrite one.
    renameButton_ = ui->buttonBox_3->addButton(tr("&Rename"), QDialogButtonBox::ActionRole);
    connect(renameButton_, &QPushButton::clicked, this, &RenameDialog::onRenameClicked);
    renameButton_->setEnabled(false); // disabled by default

    button = ui->buttonBox_2->button(QDialogButtonBox::Ignore);
    connect(button, &QPushButton::clicked, this, &RenameDialog::onIgnoreClicked);
}

RenameDialog::~RenameDialog() {
    delete ui;
}

void RenameDialog::onRenameClicked() {
    action_ = ActionRename;
    QDialog::done(QDialog::Accepted);
}

void RenameDialog::onIgnoreClicked() {
    action_ = ActionIgnore;
}

// the overwrite button
void RenameDialog::accept() {
    action_ = ActionOverwrite;
    applyToAll_ = ui->applyToAll->isChecked();
    QDialog::accept();
}

// cancel, or close the dialog
void RenameDialog::reject() {
    action_ = ActionCancel;
    QDialog::reject();
}

void RenameDialog::onFileNameChanged(QString newName) {
    newName_ = newName;
    // FIXME: check if the name already exists in the current dir
    bool hasNewName = (newName_ != oldName_);
    renameButton_->setEnabled(hasNewName);
    renameButton_->setDefault(hasNewName);

    // change default button to rename rather than overwrire
    // if the user typed a new filename
    QPushButton* overwriteButton = static_cast<QPushButton*>(ui->buttonBox_1->button(QDialogButtonBox::Ok));
    overwriteButton->setEnabled(!hasNewName);
    overwriteButton->setDefault(!hasNewName);
}


} // namespace Fm
