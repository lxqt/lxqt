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

#include "domtreeitem.h"
#include <QChildEvent>
#include <QEvent>
#include <QFile>
#include <QToolButton>


DomTreeItem::DomTreeItem(QTreeWidget *view, QWidget *widget):
    QTreeWidgetItem(view),
    mWidget(widget)
{
    init();
    mWidget->installEventFilter(this);
    connect(mWidget, SIGNAL(destroyed()), this, SLOT(widgetDestroyed()));
}


DomTreeItem::DomTreeItem(QTreeWidgetItem *parent, QWidget *widget):
    QTreeWidgetItem(parent),
    mWidget(widget)
{
    init();
    mWidget->installEventFilter(this);
    connect(mWidget, SIGNAL(destroyed()), this, SLOT(widgetDestroyed()));
}


void DomTreeItem::init()
{
    QStringList hierarcy = widgetClassHierarcy();
    for (int i=0; i<hierarcy.count(); ++i)
    {
        QString iconName = QString(QLatin1Char(':') + hierarcy.at(i)).toLower();
        if (QFile::exists(iconName))
        {
            setIcon(0, QIcon(iconName));
            break;
        }
    }

    QString text = widgetText();
    if (!text.isEmpty())
        text = QStringLiteral(" \"") + text + QStringLiteral("\"");

    QString name = mWidget->objectName();
    setText(0, QStringLiteral("%1 (%2)%3").arg(
                name ,
                widgetClassName(),
                text));
    setText(1, hierarcy.join(QStringLiteral(" :: ")));
    fill();
}


void DomTreeItem::fill()
{
    const QList<QWidget*> widgets = mWidget->findChildren<QWidget*>();
    for (QWidget *w : widgets)
    {
        if (w->parentWidget() != mWidget)
            continue;

        new DomTreeItem(this, w);
    }
}


bool DomTreeItem::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == mWidget &&
        event->type() == QEvent::ChildPolished)
    {
        QChildEvent *ce = static_cast<QChildEvent*>(event);
        QWidget *w = qobject_cast<QWidget*>(ce->child());
        if (w)
        {
            for (int i=0; i<childCount(); ++i)
            {
                DomTreeItem *ci = static_cast<DomTreeItem*>(child(i));
                if (ci->widget() == w)
                    ci->deleteLater();
            }

            new DomTreeItem(this, w);
        }
    }

    return QObject::eventFilter(watched, event);
}


QString DomTreeItem::widgetObjectName() const
{
    return mWidget->objectName();
}


QString DomTreeItem::widgetText() const
{
    QToolButton *toolButton = qobject_cast<QToolButton*>(mWidget);
    if (toolButton)
        return toolButton->text();

    return QLatin1String("");
}


QString DomTreeItem::widgetClassName() const
{
    return QString::fromUtf8(mWidget->metaObject()->className());
}


QStringList DomTreeItem::widgetClassHierarcy() const
{
    QStringList hierarcy;
    const QMetaObject *m = mWidget->metaObject();
    while (m)
    {
        hierarcy << QString::fromUtf8(m->className());
        m = m->superClass();
    }
    return hierarcy;
}


void DomTreeItem::widgetDestroyed()
{
    deleteLater();
}
