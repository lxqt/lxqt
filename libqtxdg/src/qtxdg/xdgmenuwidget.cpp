/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
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

#include "xdgmenuwidget.h"
#include "xdgicon.h"
#include "xmlhelper.h"
#include "xdgaction.h"
#include "xdgmenu.h"

#include <QEvent>
#include <QDebug>
#include <QUrl>
#include <QMimeData>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>
#include <QApplication>


class XdgMenuWidgetPrivate
{
private:
    XdgMenuWidget* const q_ptr;
    Q_DECLARE_PUBLIC(XdgMenuWidget)

public:
    explicit XdgMenuWidgetPrivate(XdgMenuWidget* parent):
        q_ptr(parent)
    {}

    void init(const QDomElement& xml);
    void buildMenu();

    QDomElement mXml;

    void mouseMoveEvent(QMouseEvent *event);

    QPoint mDragStartPosition;

private:
    XdgAction* createAction(const QDomElement& xml);
    static QString escape(QString string);
};


XdgMenuWidget::XdgMenuWidget(const XdgMenu& xdgMenu, const QString& title, QWidget* parent):
    QMenu(parent),
    d_ptr(new XdgMenuWidgetPrivate(this))
{
    d_ptr->init(xdgMenu.xml().documentElement());
    setTitle(XdgMenuWidgetPrivate::escape(title));
}


XdgMenuWidget::XdgMenuWidget(const QDomElement& menuElement, QWidget* parent):
    QMenu(parent),
    d_ptr(new XdgMenuWidgetPrivate(this))
{
    d_ptr->init(menuElement);
}


XdgMenuWidget::XdgMenuWidget(const XdgMenuWidget& other, QWidget* parent):
    QMenu(parent),
    d_ptr(new XdgMenuWidgetPrivate(this))
{
    d_ptr->init(other.d_ptr->mXml);
}


void XdgMenuWidgetPrivate::init(const QDomElement& xml)
{
    Q_Q(XdgMenuWidget);
    mXml = xml;

    q->clear();

    QString title;
    if (! xml.attribute(QLatin1String("title")).isEmpty())
        title = xml.attribute(QLatin1String("title"));
    else
        title = xml.attribute(QLatin1String("name"));
    q->setTitle(escape(title));

    //q->setToolTip(xml.attribute(QLatin1String("comment")));
    q->setToolTipsVisible(true);


    QIcon parentIcon;
    QMenu* parentMenu = qobject_cast<QMenu*>(q->parent());
    if (parentMenu)
        parentIcon = parentMenu->icon();

    q->setIcon(XdgIcon::fromTheme(mXml.attribute(QLatin1String("icon")), parentIcon));

    buildMenu();
}


XdgMenuWidget::~XdgMenuWidget()
{
    delete d_ptr;
}


XdgMenuWidget& XdgMenuWidget::operator=(const XdgMenuWidget& other)
{
    Q_D(XdgMenuWidget);
    d->init(other.d_ptr->mXml);

    return *this;
}


bool XdgMenuWidget::event(QEvent* event)
{
    Q_D(XdgMenuWidget);

    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::LeftButton)
            d->mDragStartPosition = e->pos();
    }

    else if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent *e = static_cast<QMouseEvent*>(event);
        d->mouseMoveEvent(e);
    }

    return QMenu::event(event);
}


void XdgMenuWidgetPrivate::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - mDragStartPosition).manhattanLength() < QApplication::startDragDistance())
        return;

    Q_Q(XdgMenuWidget);
    XdgAction *a = qobject_cast<XdgAction*>(q->actionAt(event->pos()));
    if (!a)
        return;

    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(a->desktopFile().fileName());

    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(urls);

    QDrag *drag = new QDrag(q);
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction | Qt::LinkAction);
}


void XdgMenuWidgetPrivate::buildMenu()
{
    Q_Q(XdgMenuWidget);

    QAction* first = nullptr;
    if (!q->actions().isEmpty())
        first = q->actions().constLast();


    DomElementIterator it(mXml, QString());
    while(it.hasNext())
    {
        QDomElement xml = it.next();

        // Build submenu ........................
        if (xml.tagName() == QLatin1String("Menu"))
            q->insertMenu(first, new XdgMenuWidget(xml, q));

        //Build application link ................
        else if (xml.tagName() == QLatin1String("AppLink"))
            q->insertAction(first, createAction(xml));

        //Build separator .......................
        else if (xml.tagName() == QLatin1String("Separator"))
            q->insertSeparator(first);

    }
}


XdgAction* XdgMenuWidgetPrivate::createAction(const QDomElement& xml)
{
    Q_Q(XdgMenuWidget);
    XdgAction* action = new XdgAction(xml.attribute(QLatin1String("desktopFile")), q);

    QString title;
    if (!xml.attribute(QLatin1String("title")).isEmpty())
        title = xml.attribute(QLatin1String("title"));
    else
        title = xml.attribute(QLatin1String("name"));

    action->setText(escape(title));

    if (!xml.attribute(QLatin1String("genericName")).isEmpty() &&
         xml.attribute(QLatin1String("genericName")) != title)
        action->setToolTip(xml.attribute(QLatin1String("genericName")));

    return action;
}


/************************************************
 This should be used when a menu item text is set
 otherwise Qt uses the &'s for creating mnemonics
 ************************************************/
QString XdgMenuWidgetPrivate::escape(QString string)
{
    return string.replace(QLatin1Char('&'), QLatin1String("&&"));
}
