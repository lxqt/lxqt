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

#ifndef FM_CREATENEWMENU_H
#define FM_CREATENEWMENU_H

#include "libfmqtglobals.h"
#include <QMenu>

#include "core/filepath.h"

namespace Fm {

class FolderView;
class Templates;
class TemplateItem;

class LIBFM_QT_API CreateNewMenu : public QMenu {
    Q_OBJECT

public:
    explicit CreateNewMenu(QWidget* dialogParent, Fm::FilePath dirPath, QWidget* parent = nullptr);
    ~CreateNewMenu() override;

protected Q_SLOTS:
    void onCreateNewFolder();

    void onCreateNewFile();

    void onCreateNew();

private Q_SLOTS:
    void addTemplateItem(const std::shared_ptr<const TemplateItem>& item);

    void updateTemplateItem(const std::shared_ptr<const TemplateItem>& oldItem, const std::shared_ptr<const TemplateItem>& newItem);

    void removeTemplateItem(const std::shared_ptr<const TemplateItem>& item);

private:
    QWidget* dialogParent_;
    Fm::FilePath dirPath_;
    QAction* templateSeparator_;
    std::shared_ptr<Templates> templates_;
};

}

#endif // FM_CREATENEWMENU_H
