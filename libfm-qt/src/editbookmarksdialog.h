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


#ifndef FM_EDITBOOKMARKSDIALOG_H
#define FM_EDITBOOKMARKSDIALOG_H

#include "libfmqtglobals.h"
#include <QDialog>
#include "core/bookmarks.h"

namespace Ui {
class EditBookmarksDialog;
}

namespace Fm {

class LIBFM_QT_API EditBookmarksDialog : public QDialog {
    Q_OBJECT
public:
    explicit EditBookmarksDialog(std::shared_ptr<Bookmarks> bookmarks, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~EditBookmarksDialog() override;

    void accept() override;

private Q_SLOTS:
    void onAddItem();
    void onRemoveItem();

private:
    Ui::EditBookmarksDialog* ui;
    std::shared_ptr<Bookmarks> bookmarks_;
};

}

#endif // FM_EDITBOOKMARKSDIALOG_H
