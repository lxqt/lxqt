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

#include "createnewmenu.h"
#include "folderview.h"
#include "utilities.h"
#include "core/iconinfo.h"
#include "core/templates.h"

#include <algorithm>

namespace Fm {


class TemplateAction: public QAction {
public:
    TemplateAction(std::shared_ptr<const TemplateItem> item, QObject* parent):
        QAction(parent) {
        setTemplateItem(std::move(item));
    }

    const std::shared_ptr<const TemplateItem> templateItem() const {
        return templateItem_;
    }

    void setTemplateItem(std::shared_ptr<const TemplateItem> item) {
        templateItem_ = std::move(item);
        auto mimeType = templateItem_->mimeType();
        setText(QStringLiteral("%1 (%2)").arg(templateItem_->displayName(),
                                              QString::fromUtf8(mimeType->desc())));
        setIcon(templateItem_->icon()->qicon());
    }

private:
    std::shared_ptr<const TemplateItem> templateItem_;
};


CreateNewMenu::CreateNewMenu(QWidget* dialogParent, Fm::FilePath dirPath, QWidget* parent):
    QMenu(parent),
    dialogParent_(dialogParent),
    dirPath_(std::move(dirPath)),
    templateSeparator_{nullptr},
    templates_{Templates::globalInstance()} {

    QAction* action = new QAction(QIcon::fromTheme(QStringLiteral("folder-new")), tr("Folder"), this);
    connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNewFolder);
    addAction(action);

    action = new QAction(QIcon::fromTheme(QStringLiteral("document-new")), tr("Blank File"), this);
    connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNewFile);
    addAction(action);

    // add more items to "Create New" menu from templates
    connect(templates_.get(), &Templates::itemAdded, this, &CreateNewMenu::addTemplateItem);
    connect(templates_.get(), &Templates::itemChanged, this, &CreateNewMenu::updateTemplateItem);
    connect(templates_.get(), &Templates::itemRemoved, this, &CreateNewMenu::removeTemplateItem);
    // when a template directory is already loaded
    templates_->forEachItem([this](const std::shared_ptr<const TemplateItem>& item) {
        addTemplateItem(item);
    });
}

CreateNewMenu::~CreateNewMenu() {
}

void CreateNewMenu::onCreateNewFile() {
    if(dirPath_) {
        createFileOrFolder(CreateNewTextFile, dirPath_, nullptr, dialogParent_);
    }
}

void CreateNewMenu::onCreateNewFolder() {
    if(dirPath_) {
        createFileOrFolder(CreateNewFolder, dirPath_, nullptr, dialogParent_);
    }
}

void CreateNewMenu::onCreateNew() {
    TemplateAction* action = static_cast<TemplateAction*>(sender());
    if(dirPath_) {
        createFileOrFolder(CreateWithTemplate, dirPath_, action->templateItem().get(), dialogParent_);
    }
}

void CreateNewMenu::addTemplateItem(const std::shared_ptr<const TemplateItem> &item) {
    if(!templateSeparator_) {
        templateSeparator_= addSeparator();
    }
    auto mimeType = item->mimeType();
    /* we support directories differently */
    if(mimeType->isDir()) {
        return;
    }

    QAction* action = new TemplateAction{item, this};
    connect(action, &QAction::triggered, this, &CreateNewMenu::onCreateNew);

    // sort actions alphabetically
    const auto allActions = actions();
    auto separatorPos = allActions.indexOf(templateSeparator_);
    if(allActions.size() == separatorPos + 1) {
        addAction(action);
    }
    else {
        // checking actions from end to start is usually faster
        for(auto i = allActions.size() - 1; i > separatorPos; --i) {
            if(QString::compare(action->text(), allActions[i]->text(), Qt::CaseInsensitive) > 0) {
                if(i == allActions.size() - 1) {
                    addAction(action);
                }
                else {
                    insertAction(allActions[i + 1], action);
                }
                return;
            }
        }
        insertAction(allActions[separatorPos + 1], action);
    }
}

void CreateNewMenu::updateTemplateItem(const std::shared_ptr<const TemplateItem> &oldItem, const std::shared_ptr<const TemplateItem> &newItem) {
    auto allActions = actions();
    auto separatorPos = allActions.indexOf(templateSeparator_);
    // all items after the separator are templates
    for(auto i = separatorPos + 1; i < allActions.size(); ++i) {
        auto action = static_cast<TemplateAction*>(allActions[i]);
        if(action->templateItem() == oldItem) {
            // update the menu item
            action->setTemplateItem(newItem);
            break;
        }
    }
}

void CreateNewMenu::removeTemplateItem(const std::shared_ptr<const TemplateItem> &item) {
    if(!templateSeparator_) {
        return;
    }
    auto allActions = actions();
    auto separatorPos = allActions.indexOf(templateSeparator_);
    // all items after the separator are templates
    for(auto i = separatorPos + 1; i < allActions.size(); ++i) {
        auto action = static_cast<TemplateAction*>(allActions[i]);
        if(action->templateItem() == item) {
            // delete the action from the menu
            removeAction(action);
            allActions.removeAt(i);
            break;
        }
    }

    // no more template items. remove the separator
    if(separatorPos == allActions.size() - 1) {
        removeAction(templateSeparator_);
        templateSeparator_ = nullptr;
    }
}

} // namespace Fm
