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


#ifndef FM_RENAMEDIALOG_H
#define FM_RENAMEDIALOG_H

#include "libfmqtglobals.h"
#include <QDialog>

#include "core/fileinfo.h"

namespace Ui {
class RenameDialog;
}

class QPushButton;

namespace Fm {

class LIBFM_QT_API RenameDialog : public QDialog {
    Q_OBJECT

public:
    enum Action {
        ActionCancel,
        ActionRename,
        ActionOverwrite,
        ActionIgnore
    };

public:
    explicit RenameDialog(const FileInfo &src, const FileInfo &dest, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~RenameDialog() override;

    Action action() {
        return action_;
    }

    bool applyToAll() {
        return applyToAll_;
    }

    QString newName() {
        return newName_;
    }

protected Q_SLOTS:
    void onRenameClicked();
    void onIgnoreClicked();
    void onFileNameChanged(QString newName);

protected:
    void accept() override;
    void reject() override;

private:
    Ui::RenameDialog* ui;
    QPushButton* renameButton_;
    Action action_;
    bool applyToAll_;
    QString oldName_;
    QString newName_;
};

}

#endif // FM_RENAMEDIALOG_H
