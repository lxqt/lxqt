/* Copyright © 2003-2007 Fredrik Höglund <fredrik@kde.org>
 * (c)GPL2
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
/*
 * additional code: Ketmar // Vampire Avalon (psyc://ketmar.no-ip.org/~Ketmar)
 */
#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include <QWidget>

#include <xcb/xcb.h>
#include <xcb/xproto.h>

class XCursorThemeData;
class PreviewCursor;

class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    PreviewWidget (QWidget *parent=0);
    ~PreviewWidget ();

    void setTheme (const XCursorThemeData *theme);
    void clearTheme ();

    QSize sizeHint () const;

    void setCursorHandle(xcb_cursor_t cursorHandle);
    void setCursorSize(int size);
    int getCursorSize();
    void setCurrentCursorSize(int size);
    int getCurrentCursorSize();

protected:
    void paintEvent (QPaintEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void resizeEvent (QResizeEvent *e);

private:
    void layoutItems ();

    QList<PreviewCursor *> mList;
    const PreviewCursor *mCurrent;
    bool mNeedLayout;
    int mCursorSize;
    int mCurrentCursorSize;
    const XCursorThemeData *mTheme;
};

#endif
