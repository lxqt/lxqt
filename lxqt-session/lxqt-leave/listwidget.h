/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2017 LXQt team
 * Authors:
 *   Palo Kisa <palo.kisa@gmail.com>
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
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <QListWidget>

/*!
 * Single purpose QListWidget with unified item sizes (based on the
 * biggest item) and showing rows x columns items.
 *
 * \note It expects that items aren't ever changed after show().
 */
class ListWidget : public QListWidget
{
public:
    ListWidget(QWidget * parent = nullptr);
    void setRows(int rows);
    void setColumns(int columns);

protected:
    QSize viewportSizeHint() const override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    void keyPressEvent(QKeyEvent * event) override;
    void focusInEvent(QFocusEvent * event) override;

private:
    int mRows;
    int mColumns;
};
