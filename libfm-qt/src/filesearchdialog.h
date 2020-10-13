/*
 * Copyright (C) 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef FM_FILESEARCHDIALOG_H
#define FM_FILESEARCHDIALOG_H

#include "libfmqtglobals.h"
#include <QDialog>
#include "core/filepath.h"

namespace Ui {
class SearchDialog;
}

namespace Fm {

class LIBFM_QT_API FileSearchDialog : public QDialog {
public:
    explicit FileSearchDialog(QStringList paths = QStringList(), QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~FileSearchDialog() override;

    const FilePath& searchUri() const {
        return searchUri_;
    }

    void accept() override;

    bool nameCaseInsensitive() const;
    void setNameCaseInsensitive(bool caseInsensitive);

    bool contentCaseInsensitive() const;
    void setContentCaseInsensitive(bool caseInsensitive);

    bool nameRegexp() const;
    void setNameRegexp(bool reg);

    bool contentRegexp() const;
    void setContentRegexp(bool reg);

    bool recursive() const;
    void setRecursive(bool rec);

    bool searchhHidden() const;
    void setSearchhHidden(bool hidden);

private Q_SLOTS:
    void onAddPath();
    void onRemovePath();

private:
    Ui::SearchDialog* ui;
    FilePath searchUri_;
};

}

#endif // FM_FILESEARCHDIALOG_H
