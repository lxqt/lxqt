/*
 * Copyright (C) 2012 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright (C) 2012 - 2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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


#include "foldermenu.h"
#include "createnewmenu.h"
#include "filepropsdialog.h"
#include "folderview.h"
#include "utilities.h"
#include <cstring> // for memset
#include <QDebug>
#include "customaction_p.h"
#include "customactions/fileaction.h"
#include <QMessageBox>

namespace Fm {

FolderMenu::FolderMenu(FolderView* view, QWidget* parent):
    QMenu(parent),
    view_(view) {

    ProxyFolderModel* model = view_->model();

    createAction_ = new QAction(tr("Create &New"), this);
    addAction(createAction_);

    createAction_->setMenu(new CreateNewMenu(view_, view_->path(), this));

    separator1_ = addSeparator();

    pasteAction_ = new QAction(QIcon::fromTheme(QStringLiteral("edit-paste")), tr("&Paste"), this);
    addAction(pasteAction_);
    connect(pasteAction_, &QAction::triggered, this, &FolderMenu::onPasteActionTriggered);

    separator2_ = addSeparator();

    selectAllAction_ = new QAction(tr("Select &All"), this);
    addAction(selectAllAction_);
    connect(selectAllAction_, &QAction::triggered, this, &FolderMenu::onSelectAllActionTriggered);

    invertSelectionAction_ = new QAction(tr("Invert Selection"), this);
    addAction(invertSelectionAction_);
    connect(invertSelectionAction_, &QAction::triggered, this, &FolderMenu::onInvertSelectionActionTriggered);

    separator3_ = addSeparator();

    sortAction_ = new QAction(tr("Sorting"), this);
    addAction(sortAction_);
    createSortMenu();
    sortAction_->setMenu(sortMenu_);

    showHiddenAction_ = new QAction(tr("Show Hidden"), this);
    addAction(showHiddenAction_);
    showHiddenAction_->setCheckable(true);
    showHiddenAction_->setChecked(model->showHidden());
    connect(showHiddenAction_, &QAction::triggered, this, &FolderMenu::onShowHiddenActionTriggered);

    auto folderInfo = view_->folderInfo();
    if(folderInfo) { // should never be null (see FolderView::onFileClicked)
        // DES-EMA custom actions integration
        FileInfoList files;
        files.push_back(folderInfo);
        auto custom_actions = FileActionItem::get_actions_for_files(files);
        for(auto& item: custom_actions) {
            if(item && !(item->get_target() & FILE_ACTION_TARGET_CONTEXT)) {
                continue;  // this item is not for context menu
            }
            if(item == custom_actions.front() && item && !item->is_action()) {
                addSeparator(); // before all custom actions
            }
            addCustomActionItem(this, item);
        }

        // disable actons that can't be used
        pasteAction_->setEnabled(folderInfo->isWritable());
        createAction_->setEnabled(folderInfo->isWritable());
    }

    separator4_ = addSeparator();

    propertiesAction_ = new QAction(tr("Folder Pr&operties"), this);
    addAction(propertiesAction_);
    connect(propertiesAction_, &QAction::triggered, this, &FolderMenu::onPropertiesActionTriggered);
}

FolderMenu::~FolderMenu() {
}

void FolderMenu::addCustomActionItem(QMenu* menu, std::shared_ptr<const FileActionItem> item) {
    if(!item) {
        return;
    }
    if(item->is_action() && !(item->get_target() & FILE_ACTION_TARGET_CONTEXT)) {
        return;
    }

    CustomAction* action = new CustomAction(item, menu);
    menu->addAction(action);
    if(item->is_menu()) {
        auto& subitems = item->get_sub_items();
        if(!subitems.empty()) {
            QMenu* submenu = new QMenu(menu);
            for(auto& subitem: subitems) {
                addCustomActionItem(submenu, subitem);
            }
            action->setMenu(submenu);
        }
    }
    else if(item->is_action()) {
        connect(action, &QAction::triggered, this, &FolderMenu::onCustomActionTrigerred);
    }
}

void FolderMenu::onCustomActionTrigerred() {
    CustomAction* action = static_cast<CustomAction*>(sender());
    auto& item = action->item();
    auto folderInfo = view_->folderInfo();
    if(folderInfo) {
        CStrPtr output;
        FileInfoList file_list;
        file_list.push_back(folderInfo);
        item->launch(nullptr, file_list, output);
        if(output) {
            QMessageBox::information(this, tr("Output"), QString::fromUtf8(output.get()));
        }
    }
}

void FolderMenu::addSortMenuItem(const QString &title, int id) {
    QAction* action = new QAction(title, this);
    action->setData(QVariant(id));
    sortMenu_->addAction(action);
    action->setCheckable(true);
    action->setChecked(id == view_->model()->sortColumn());
    sortActionGroup_->addAction(action);
    connect(action, &QAction::triggered, this, &FolderMenu::onSortActionTriggered);
}

void FolderMenu::createSortMenu() {
    ProxyFolderModel* model = view_->model();

    sortMenu_ = new QMenu(this);
    sortActionGroup_ = new QActionGroup(sortMenu_);
    sortActionGroup_->setExclusive(true);

    addSortMenuItem(tr("By File Name"), FolderModel::ColumnFileName);
    addSortMenuItem(tr("By Modification Time"), FolderModel::ColumnFileMTime);
    if(auto folderPath = view_->path()) {
        if(strcmp(folderPath.toString().get(), "trash:///") == 0) {
            addSortMenuItem(tr("By Deletion Time"), FolderModel::ColumnFileDTime);
        }
    }
    addSortMenuItem(tr("By File Size"), FolderModel::ColumnFileSize);
    addSortMenuItem(tr("By File Type"), FolderModel::ColumnFileType);
    addSortMenuItem(tr("By File Owner"), FolderModel::ColumnFileOwner);
    addSortMenuItem(tr("By File Group"), FolderModel::ColumnFileGroup);

    sortMenu_->addSeparator();

    QActionGroup* group = new QActionGroup(this);
    group->setExclusive(true);
    actionAscending_ = new QAction(tr("Ascending"), this);
    actionAscending_->setCheckable(true);
    sortMenu_->addAction(actionAscending_);
    group->addAction(actionAscending_);

    actionDescending_ = new QAction(tr("Descending"), this);
    actionDescending_->setCheckable(true);
    sortMenu_->addAction(actionDescending_);
    group->addAction(actionDescending_);

    if(model->sortOrder() == Qt::AscendingOrder) {
        actionAscending_->setChecked(true);
    }
    else {
        actionDescending_->setChecked(true);
    }

    connect(actionAscending_, &QAction::triggered, this, &FolderMenu::onSortOrderActionTriggered);
    connect(actionDescending_, &QAction::triggered, this, &FolderMenu::onSortOrderActionTriggered);

    sortMenu_->addSeparator();

    QAction* actionFolderFirst = new QAction(tr("Folder First"), this);
    sortMenu_->addAction(actionFolderFirst);
    actionFolderFirst->setCheckable(true);
    if(model->folderFirst()) {
        actionFolderFirst->setChecked(true);
    }
    connect(actionFolderFirst, &QAction::triggered, this, &FolderMenu::onFolderFirstActionTriggered);

    QAction* actionHiddenLast = new QAction(tr("Hidden Last"), this);
    sortMenu_->addAction(actionHiddenLast);
    actionHiddenLast->setCheckable(true);
    if(model->hiddenLast()) {
        actionHiddenLast->setChecked(true);
    }
    connect(actionHiddenLast, &QAction::triggered, this, &FolderMenu::onHiddenLastActionTriggered);

    QAction* actionCaseSensitive = new QAction(tr("Case Sensitive"), this);
    sortMenu_->addAction(actionCaseSensitive);
    actionCaseSensitive->setCheckable(true);

    if(model->sortCaseSensitivity() == Qt::CaseSensitive) {
        actionCaseSensitive->setChecked(true);
    }

    connect(actionCaseSensitive, &QAction::triggered, this, &FolderMenu::onCaseSensitiveActionTriggered);
}

void FolderMenu::onPasteActionTriggered() {
    auto folderPath = view_->path();
    if(folderPath) {
        pasteFilesFromClipboard(folderPath);
    }
}

void FolderMenu::onSelectAllActionTriggered() {
    view_->selectAll();
}

void FolderMenu::onInvertSelectionActionTriggered() {
    view_->invertSelection();
}

void FolderMenu::onSortActionTriggered(bool /*checked*/) {
    ProxyFolderModel* model = view_->model();

    if(model && sortActionGroup_) {
        QAction* action = static_cast<QAction*>(sender());

        const auto actions = sortActionGroup_->actions();
        for(auto a : actions) {
            if(a == action) {
                int col = a->data().toInt();
                if(col >= 0 && col < FolderModel::NumOfColumns) {
                    model->sort(col, model->sortOrder());
                }
                break;
            }
        }
    }
}

void FolderMenu::onSortOrderActionTriggered(bool /*checked*/) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        QAction* action = static_cast<QAction*>(sender());
        Qt::SortOrder order;

        if(action == actionAscending_) {
            order = Qt::AscendingOrder;
        }
        else {
            order = Qt::DescendingOrder;
        }

        model->sort(model->sortColumn(), order);
    }
}

void FolderMenu::onShowHiddenActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        qDebug("show hidden: %d", checked);
        model->setShowHidden(checked);
    }
}

void FolderMenu::onCaseSensitiveActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        model->setSortCaseSensitivity(checked ? Qt::CaseSensitive : Qt::CaseInsensitive);
    }
}

void FolderMenu::onFolderFirstActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        model->setFolderFirst(checked);
    }
}

void FolderMenu::onHiddenLastActionTriggered(bool checked) {
    ProxyFolderModel* model = view_->model();

    if(model) {
        model->setHiddenLast(checked);
    }
}

void FolderMenu::onPropertiesActionTriggered() {
    auto folderInfo = view_->folderInfo();
    if(folderInfo) {
        FilePropsDialog::showForFile(folderInfo);
    }
}

} // namespace Fm
