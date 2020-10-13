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


#ifndef FM_PATHEDIT_H
#define FM_PATHEDIT_H

#include "libfmqtglobals.h"
#include <QLineEdit>
#include <gio/gio.h>

class QCompleter;
class QStringListModel;

namespace Fm {

class PathEditJob;

class LIBFM_QT_API PathEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit PathEdit(QWidget* parent = nullptr);
    ~PathEdit() override;

protected:
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    bool event(QEvent* e) override;

private Q_SLOTS:
    void onTextChanged(const QString& text);
    void onTextEdited(const QString& text);

private:
    void selectNextCompletionRow(bool downward);
    void reloadCompleter(bool triggeredByFocusInEvent = false);
    void freeCompleter();
    void onJobFinished();

private:
    QCompleter* completer_;
    QStringListModel* model_;
    QString currentPrefix_;
    GCancellable* cancellable_;
    QString lastTypedText_;
};

}

#endif // FM_PATHEDIT_H
