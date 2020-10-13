/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Dmitriy Zhukov <zjesclean@gmail.com>
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
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#ifndef _CONTENT_H_
#define _CONTENT_H_

#include <QWidget>
#include "controls.h"

class QLabel;
class QToolButton;

class Content : public QWidget
{
    Q_OBJECT
public:
    Content(bool layoutEnabled);
    ~Content();

public:
    void setEnabled(Controls cnt, bool enabled);
    QWidget* widget(Controls cnt) const;
    bool setup();

    virtual bool eventFilter(QObject *object, QEvent *event);

    void showHorizontal();
    void showVertical();
public slots:
    void layoutChanged(const QString & sym, const QString & name, const QString & variant);
    void modifierStateChanged(Controls mod, bool active);
signals:
    void controlClicked(Controls cnt);
private:
    bool        m_layoutEnabled;
    QString     m_layoutFlagPattern;
    QLabel     *m_capsLock;
    QLabel     *m_numLock;
    QLabel     *m_scrollLock;
    QToolButton *m_layout;
};

#endif
