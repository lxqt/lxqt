/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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


#include "pluginmoveprocessor.h"
#include "plugin.h"
#include "lxqtpanellayout.h"
#include <QMouseEvent>


/************************************************

 ************************************************/
PluginMoveProcessor::PluginMoveProcessor(LXQtPanelLayout *layout, Plugin *plugin):
    QWidget(plugin),
    mLayout(layout),
    mPlugin(plugin)
{
    mDestIndex = mLayout->indexOf(plugin);

    grabKeyboard();
}


/************************************************

 ************************************************/
PluginMoveProcessor::~PluginMoveProcessor()
{
}


/************************************************

 ************************************************/
void PluginMoveProcessor::start()
{
    // We have not memoryleaks there.
    // The animation will be automatically deleted when stopped.
    CursorAnimation *cursorAnimation = new CursorAnimation();
    connect(cursorAnimation, SIGNAL(finished()), this, SLOT(doStart()));
    cursorAnimation->setEasingCurve(QEasingCurve::InOutQuad);
    cursorAnimation->setDuration(150);

    cursorAnimation->setStartValue(QCursor::pos());
    cursorAnimation->setEndValue(mPlugin->mapToGlobal(mPlugin->rect().center()));
    cursorAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}


/************************************************

 ************************************************/
void PluginMoveProcessor::doStart()
{
    setMouseTracking(true);
    show(); //  Only visible widgets can grab mouse input.
    grabMouse(mLayout->isHorizontal() ? Qt::SizeHorCursor : Qt::SizeVerCursor);
}


/************************************************

 ************************************************/
void PluginMoveProcessor::mouseMoveEvent(QMouseEvent *event)
{
    QPoint mouse = mLayout->parentWidget()->mapFromGlobal(event->globalPos());

    MousePosInfo pos = itemByMousePos(mouse);

    QLayoutItem *prevItem = 0;
    QLayoutItem *nextItem = 0;
    if (pos.after)
    {
        mDestIndex = pos.index + 1;
        prevItem = pos.item;
        nextItem = mLayout->itemAt(pos.index + 1);
    }
    else
    {
        prevItem = mLayout->itemAt(pos.index - 1);
        nextItem = pos.item;
        mDestIndex = pos.index;
    }

    bool plugSep = mPlugin->isSeparate();
    bool prevSep = LXQtPanelLayout::itemIsSeparate(prevItem);
    bool nextSep = LXQtPanelLayout::itemIsSeparate(nextItem);

    if (!nextItem)
    {
        if (mLayout->isHorizontal())
            drawMark(prevItem, prevSep ? RightMark : BottomMark);
        else
            drawMark(prevItem, prevSep ? BottomMark : RightMark);
        return;
    }

    if (mLayout->lineCount() == 1)
    {
        if (mLayout->isHorizontal())
            drawMark(nextItem, LeftMark);
        else
            drawMark(nextItem, TopMark);
        return;
    }


    if (!prevItem)
    {
        if (mLayout->isHorizontal())
            drawMark(nextItem, nextSep ? LeftMark : TopMark);
        else
            drawMark(nextItem, nextSep ? TopMark : LeftMark);
        return;
    }

    // We prefer to draw line at the top/left of next item.
    // But if next item and moved plugin have different types (separate an not) and
    // previous item hase same type as moved plugin we draw line at the end of previous one.
    if (plugSep != nextSep && plugSep == prevSep)
    {
        if (mLayout->isHorizontal())
            drawMark(prevItem, prevSep ? RightMark : BottomMark);
        else
            drawMark(prevItem, prevSep ? BottomMark : RightMark);
    }
    else
    {
        if (mLayout->isHorizontal())
            drawMark(nextItem, nextSep ? LeftMark : TopMark);
        else
            drawMark(nextItem, nextSep ? TopMark : LeftMark);
    }
}


/************************************************

 ************************************************/
PluginMoveProcessor::MousePosInfo PluginMoveProcessor::itemByMousePos(const QPoint mouse) const
{
    MousePosInfo ret;

    for (int i = mLayout->count()-1; i > -1; --i)
    {
        QLayoutItem *item = mLayout->itemAt(i);
        QRect itemRect = item->geometry();
        if (mouse.x() > itemRect.left() &&
            mouse.y() > itemRect.top())
        {

            ret.index = i;
            ret.item = item;
            if (mLayout->isHorizontal())
            {
                ret.after = LXQtPanelLayout::itemIsSeparate(item) ?
                        mouse.x() > itemRect.center().x() :
                        mouse.y() > itemRect.center().y() ;
            }
            else
            {
                ret.after = LXQtPanelLayout::itemIsSeparate(item) ?
                        mouse.y() > itemRect.center().y() :
                        mouse.x() > itemRect.center().x() ;
            }
            return ret;
        }
    }

    ret.index = 0;
    ret.item = mLayout->itemAt(0);
    ret.after = false;
    return ret;
}


/************************************************

 ************************************************/
void PluginMoveProcessor::drawMark(QLayoutItem *item, MarkType markType)
{
    QWidget *widget = (item) ? item->widget() : 0;

    static QWidget *prevWidget = 0;
    if (prevWidget && prevWidget != widget)
        prevWidget->setStyleSheet(QLatin1String(""));

    prevWidget = widget;

    if (!widget)
        return;

    QString border1;
    QString border2;
    switch(markType)
    {
    case TopMark:
        border1 = QLatin1String("top");
        border2 = QLatin1String("bottom");
        break;

    case BottomMark:
        border1 = QLatin1String("bottom");
        border2 = QLatin1String("top");
        break;

    case LeftMark:
        border1 = QLatin1String("left");
        border2 = QLatin1String("right");
        break;

    case RightMark:
        border1 = QLatin1String("right");
        border2 = QLatin1String("left");
        break;

    }

    widget->setStyleSheet(QString::fromLatin1("#%1 {"
                                  "border-%2: 2px solid rgba(%4, %5, %6, %7); "
                                  "border-%3: -2px solid; "
                                  "background-color: transparent; }")
                          .arg(widget->objectName())
                          .arg(border1)
                          .arg(border2)
                          .arg(Plugin::moveMarkerColor().red())
                          .arg(Plugin::moveMarkerColor().green())
                          .arg(Plugin::moveMarkerColor().blue())
                          .arg(Plugin::moveMarkerColor().alpha())
                          );
}


/************************************************

 ************************************************/
void PluginMoveProcessor::mousePressEvent(QMouseEvent *event)
{
    event->accept();
}


/************************************************

 ************************************************/
void PluginMoveProcessor::mouseReleaseEvent(QMouseEvent *event)
{
    event->accept();
    releaseMouse();
    setMouseTracking(false);
    doFinish(false);
}

/************************************************

 ************************************************/
void PluginMoveProcessor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        doFinish(true);
        return;
    }
    QWidget::keyPressEvent(event);
}

/************************************************

 ************************************************/
void PluginMoveProcessor::doFinish(bool cancel)
{
    releaseKeyboard();

    drawMark(0, TopMark);

    if (!cancel)
    {
        int currentIdx = mLayout->indexOf(mPlugin);
        if (currentIdx == mDestIndex)
            return;

        if (mDestIndex > currentIdx)
            mDestIndex--;

        mLayout->moveItem(currentIdx, mDestIndex, true);
    }

    emit finished();
    deleteLater();
}
