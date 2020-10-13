/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include "quicklaunchbutton.h"
#include "lxqtquicklaunch.h"
#include "../panel/ilxqtpanelplugin.h"
#include <QAction>
#include <QDrag>
#include <QMenu>
#include <QMouseEvent>
#include <QStylePainter>
#include <QStyleOptionToolButton>
#include <QApplication>
#include <XdgIcon>

#define MIMETYPE "x-lxqt/quicklaunch-button"


QuickLaunchButton::QuickLaunchButton(QuickLaunchAction * act, ILXQtPanelPlugin * plugin, QWidget * parent)
    : QToolButton(parent),
      mAct(act),
      mPlugin(plugin)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAcceptDrops(true);
    setAutoRaise(true);

    setDefaultAction(mAct);
    mAct->setParent(this);

    const QString dndStr = QStringLiteral(" ") + tr("(Ctrl + DND)");

    mMoveLeftAct = new QAction(XdgIcon::fromTheme(QStringLiteral("go-previous")), tr("Move left") + dndStr, this);
    connect(mMoveLeftAct, SIGNAL(triggered()), this, SIGNAL(movedLeft()));

    mMoveRightAct = new QAction(XdgIcon::fromTheme(QStringLiteral("go-next")), tr("Move right") + dndStr, this);
    connect(mMoveRightAct, SIGNAL(triggered()), this, SIGNAL(movedRight()));


    mDeleteAct = new QAction(XdgIcon::fromTheme(QStringLiteral("dialog-close")), tr("Remove from quicklaunch"), this);
    connect(mDeleteAct, SIGNAL(triggered()), this, SLOT(selfRemove()));

    mMenu = new QMenu(this);
    mMenu->addAction(mAct);
    mMenu->addActions(mAct->addtitionalActions());
    mMenu->addSeparator();
    mMenu->addAction(mMoveLeftAct);
    mMenu->addAction(mMoveRightAct);
    mMenu->addSeparator();
    mMenu->addAction(mDeleteAct);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(this_customContextMenuRequested(const QPoint&)));
}


QuickLaunchButton::~QuickLaunchButton()
{
    //m_act->deleteLater();
}


QHash<QString,QString> QuickLaunchButton::settingsMap()
{
    Q_ASSERT(mAct);
    return mAct->settingsMap();
}


void QuickLaunchButton::this_customContextMenuRequested(const QPoint & /*pos*/)
{
    LXQtQuickLaunch *panel = qobject_cast<LXQtQuickLaunch*>(parent());

    mMoveLeftAct->setEnabled(!mPlugin->panel()->isLocked() && panel && panel->indexOfButton(this) > 0);
    mMoveRightAct->setEnabled(!mPlugin->panel()->isLocked() && panel && panel->indexOfButton(this) < panel->countOfButtons() - 1);
    mDeleteAct->setEnabled(!mPlugin->panel()->isLocked());
    mPlugin->willShowWindow(mMenu);
    mMenu->popup(mPlugin->panel()->calculatePopupWindowPos(mapToGlobal({0, 0}), mMenu->sizeHint()).topLeft());
}


void QuickLaunchButton::selfRemove()
{
    emit buttonDeleted();
}


void QuickLaunchButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->modifiers() == Qt::ControlModifier)
    {
        mDragStart = e->pos();
        return;
    }

    QToolButton::mousePressEvent(e);
}


void QuickLaunchButton::mouseMoveEvent(QMouseEvent *e)
{
    if (mPlugin->panel()->isLocked() || !(e->buttons() & Qt::LeftButton))
    {
        return;
    }

    if ((e->pos() - mDragStart).manhattanLength() < QApplication::startDragDistance())
    {
        return;
    }

    if (e->modifiers() != Qt::ControlModifier)
    {
        return;
    }

    QDrag *drag = new QDrag(this);
    ButtonMimeData *mimeData = new ButtonMimeData();
    mimeData->setButton(this);
    drag->setMimeData(mimeData);

    drag->exec(Qt::MoveAction);

    // Icon was droped outside the panel, remove button
    if (!drag->target())
    {
        selfRemove();
    }
}


void QuickLaunchButton::dragMoveEvent(QDragMoveEvent * e)
{
    if (!mPlugin->panel()->isLocked() && e->mimeData()->hasFormat(QStringLiteral(MIMETYPE)))
        e->acceptProposedAction();
    else
        e->ignore();
}


void QuickLaunchButton::dragEnterEvent(QDragEnterEvent *e)
{
    if (!mPlugin->panel()->isLocked()) {
        const ButtonMimeData *mimeData = qobject_cast<const ButtonMimeData*>(e->mimeData());
        if (mimeData && mimeData->button())
        {
            emit switchButtons(mimeData->button(), this);
        }
    }
}
