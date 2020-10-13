/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2012 Razor team
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


#include "popupmenu.h"
#include <QWidgetAction>
#include <QToolButton>
#include <QEvent>
#include <QKeyEvent>

static const char POPUPMENU_TITLE[] = "POPUP_MENU_TITLE_OBJECT_NAME";

/************************************************

 ************************************************/
QAction* PopupMenu::addTitle(const QIcon &icon, const QString &text)
{
    QAction *buttonAction = new QAction(this);
    QFont font = buttonAction->font();
    font.setBold(true);
    buttonAction->setText(QString(text).replace(QLatin1String("&"), QLatin1String("&&")));
    buttonAction->setFont(font);
    buttonAction->setIcon(icon);

    QWidgetAction *action = new QWidgetAction(this);
    action->setObjectName(QLatin1String(POPUPMENU_TITLE));
    QToolButton *titleButton = new QToolButton(this);
    titleButton->installEventFilter(this); // prevent clicks on the title of the menu
    titleButton->setDefaultAction(buttonAction);
    titleButton->setDown(true); // prevent hover style changes in some styles
    titleButton->setCheckable(true);
    titleButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    action->setDefaultWidget(titleButton);

    addAction(action);
    return action;
}


/************************************************

 ************************************************/
QAction* PopupMenu::addTitle(const QString &text)
{
    return addTitle(QIcon(), text);
}


/************************************************

 ************************************************/
bool PopupMenu::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object);

    if (event->type() == QEvent::Paint ||
        event->type() == QEvent::KeyPress ||
        event->type() == QEvent::KeyRelease
       )
    {
        return false;
    }

    event->accept();
    return true;
}


/************************************************

 ************************************************/
void PopupMenu::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down)
    {
        QMenu::keyPressEvent(e);

        QWidgetAction *action = qobject_cast<QWidgetAction*>(this->activeAction());
        QWidgetAction *firstAction = action;

        while (action && action->objectName() == QLatin1String(POPUPMENU_TITLE))
        {
            this->keyPressEvent(e);
            action = qobject_cast<QWidgetAction*>(this->activeAction());

            if (firstAction == action) // we looped and only found titles
            {
                this->setActiveAction(0);
                break;
            }
        }

        return;
    }

    QMenu::keyPressEvent(e);
}


