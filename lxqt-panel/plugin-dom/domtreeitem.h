/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
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

#ifndef DOMTREEITEM_H
#define DOMTREEITEM_H

#include <QObject>
#include <QTreeWidgetItem>


class DomTreeItem: public QObject, public QTreeWidgetItem
{
    Q_OBJECT
public:
    explicit DomTreeItem(QTreeWidget *view, QWidget *widget);
    explicit DomTreeItem(QTreeWidgetItem *parent, QWidget *widget);
    bool eventFilter(QObject *watched, QEvent *event);

    QString widgetObjectName() const;
    QString widgetText() const;
    QString widgetClassName() const;
    QStringList widgetClassHierarcy() const;
    QWidget *widget() const { return mWidget; }

private slots:
    void widgetDestroyed();

private:
    QWidget *mWidget;
    void init();
    void fill();
};

#endif // DOMTREEITEM_H
