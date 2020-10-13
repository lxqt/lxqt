/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 *            2014 LXQt team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.com>
 *   Maciej Płaza <plaza.maciej@gmail.com>
 *   Kuzma Shapran <kuzma.shapran@gmail.com>
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

#include <QApplication>
#include <QDebug>
#include <QSignalMapper>
#include <QToolButton>
#include <QSettings>
#include <QList>
#include <QMimeData>
#include <QWheelEvent>
#include <QFlag>
#include <QX11Info>
#include <QTimer>

#include <lxqt-globalkeys.h>
#include <LXQt/GridLayout>
#include <XdgIcon>

#include "lxqttaskbar.h"
#include "lxqttaskgroup.h"

using namespace LXQt;

/************************************************

************************************************/
LXQtTaskBar::LXQtTaskBar(ILXQtPanelPlugin *plugin, QWidget *parent) :
    QFrame(parent),
    mSignalMapper(new QSignalMapper(this)),
    mButtonStyle(Qt::ToolButtonTextBesideIcon),
    mButtonWidth(400),
    mButtonHeight(100),
    mCloseOnMiddleClick(true),
    mRaiseOnCurrentDesktop(true),
    mShowOnlyOneDesktopTasks(false),
    mShowDesktopNum(0),
    mShowOnlyCurrentScreenTasks(false),
    mShowOnlyMinimizedTasks(false),
    mAutoRotate(true),
    mGroupingEnabled(true),
    mShowGroupOnHover(true),
    mUngroupedNextToExisting(false),
    mIconByClass(false),
    mWheelEventsAction(1),
    mWheelDeltaThreshold(300),
    mPlugin(plugin),
    mPlaceHolder(new QWidget(this)),
    mStyle(new LeftAlignedTextStyle())
{
    setStyle(mStyle);
    mLayout = new LXQt::GridLayout(this);
    setLayout(mLayout);
    mLayout->setMargin(0);
    mLayout->setStretch(LXQt::GridLayout::StretchHorizontal | LXQt::GridLayout::StretchVertical);
    realign();

    mPlaceHolder->setMinimumSize(1, 1);
    mPlaceHolder->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    mPlaceHolder->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    mLayout->addWidget(mPlaceHolder);

    QTimer::singleShot(0, this, SLOT(settingsChanged()));
    setAcceptDrops(true);

    connect(mSignalMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped), this, &LXQtTaskBar::activateTask);
    QTimer::singleShot(0, this, &LXQtTaskBar::registerShortcuts);

    connect(KWindowSystem::self(), static_cast<void (KWindowSystem::*)(WId, NET::Properties, NET::Properties2)>(&KWindowSystem::windowChanged)
            , this, &LXQtTaskBar::onWindowChanged);
    connect(KWindowSystem::self(), &KWindowSystem::windowAdded, this, &LXQtTaskBar::onWindowAdded);
    connect(KWindowSystem::self(), &KWindowSystem::windowRemoved, this, &LXQtTaskBar::onWindowRemoved);
}

/************************************************

 ************************************************/
LXQtTaskBar::~LXQtTaskBar()
{
    delete mStyle;
}

/************************************************

 ************************************************/
bool LXQtTaskBar::acceptWindow(WId window) const
{
    QFlags<NET::WindowTypeMask> ignoreList;
    ignoreList |= NET::DesktopMask;
    ignoreList |= NET::DockMask;
    ignoreList |= NET::SplashMask;
    ignoreList |= NET::ToolbarMask;
    ignoreList |= NET::MenuMask;
    ignoreList |= NET::PopupMenuMask;
    ignoreList |= NET::NotificationMask;

    KWindowInfo info(window, NET::WMWindowType | NET::WMState, NET::WM2TransientFor);
    if (!info.valid())
        return false;

    if (NET::typeMatchesMask(info.windowType(NET::AllTypesMask), ignoreList))
        return false;

    if (info.state() & NET::SkipTaskbar)
        return false;

    // WM_TRANSIENT_FOR hint not set - normal window
    WId transFor = info.transientFor();
    if (transFor == 0 || transFor == window || transFor == (WId) QX11Info::appRootWindow())
        return true;

    info = KWindowInfo(transFor, NET::WMWindowType);

    QFlags<NET::WindowTypeMask> normalFlag;
    normalFlag |= NET::NormalMask;
    normalFlag |= NET::DialogMask;
    normalFlag |= NET::UtilityMask;

    return !NET::typeMatchesMask(info.windowType(NET::AllTypesMask), normalFlag);
}

/************************************************

 ************************************************/
void LXQtTaskBar::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat(LXQtTaskGroup::mimeDataFormat()))
    {
        event->acceptProposedAction();
        buttonMove(nullptr, qobject_cast<LXQtTaskGroup *>(event->source()), event->pos());
    } else
        event->ignore();
    QWidget::dragEnterEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::dragMoveEvent(QDragMoveEvent * event)
{
    //we don't get any dragMoveEvents if dragEnter wasn't accepted
    buttonMove(nullptr, qobject_cast<LXQtTaskGroup *>(event->source()), event->pos());
    QWidget::dragMoveEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::buttonMove(LXQtTaskGroup * dst, LXQtTaskGroup * src, QPoint const & pos)
{
    int src_index;
    if (!src || -1 == (src_index = mLayout->indexOf(src)))
    {
        qDebug() << "Dropped invalid";
        return;
    }

    const int size = mLayout->count();
    Q_ASSERT(0 < size);
    //dst is nullptr in case the drop occured on empty space in taskbar
    int dst_index;
    if (nullptr == dst)
    {
        //moving based on taskbar (not signaled by button)
        QRect occupied = mLayout->occupiedGeometry();
        QRect last_empty_row{occupied};
        const QRect last_item_geometry = mLayout->itemAt(size - 1)->geometry();
        if (mPlugin->panel()->isHorizontal())
        {
            if (isRightToLeft())
            {
                last_empty_row.setTopRight(last_item_geometry.topLeft());
            } else
            {
                last_empty_row.setTopLeft(last_item_geometry.topRight());
            }
        } else
        {
            if (isRightToLeft())
            {
                last_empty_row.setTopRight(last_item_geometry.topRight());
            } else
            {
                last_empty_row.setTopLeft(last_item_geometry.topLeft());
            }
        }
        if (occupied.contains(pos) && !last_empty_row.contains(pos))
            return;

        dst_index = size;
    } else
    {
        //moving based on signal from child button
        dst_index = mLayout->indexOf(dst);
    }

    //moving lower index to higher one => consider as the QList::move => insert(to, takeAt(from))
    if (src_index < dst_index)
    {
        if (size == dst_index
                || src_index + 1 != dst_index)
        {
            --dst_index;
        } else
        {
            //switching positions of next standing
            const int tmp_index = src_index;
            src_index = dst_index;
            dst_index = tmp_index;
        }
    }

    if (dst_index == src_index
            || mLayout->animatedMoveInProgress()
       )
        return;

    mLayout->moveItem(src_index, dst_index, true);
}

/************************************************

 ************************************************/
void LXQtTaskBar::groupBecomeEmptySlot()
{
    //group now contains no buttons - clean up in hash and delete the group
    LXQtTaskGroup * const group = qobject_cast<LXQtTaskGroup*>(sender());
    Q_ASSERT(group);

    for (auto i = mKnownWindows.begin(); mKnownWindows.end() != i; )
    {
        if (group == *i)
            i = mKnownWindows.erase(i);
        else
            ++i;
    }
    mLayout->removeWidget(group);
    group->deleteLater();
}

/************************************************

 ************************************************/
void LXQtTaskBar::addWindow(WId window)
{
    // If grouping disabled group behaves like regular button
    const QString group_id = mGroupingEnabled ? QString::fromUtf8(KWindowInfo(window, NET::Properties(), NET::WM2WindowClass).windowClassClass()) : QString::number(window);

    LXQtTaskGroup *group = nullptr;
    auto i_group = mKnownWindows.find(window);
    if (mKnownWindows.end() != i_group)
    {
        if ((*i_group)->groupName() == group_id)
            group = *i_group;
        else
            (*i_group)->onWindowRemoved(window);
    }

    //check if window belongs to some existing group
    if (!group && mGroupingEnabled)
    {
        for (auto i = mKnownWindows.cbegin(), i_e = mKnownWindows.cend(); i != i_e; ++i)
        {
            if ((*i)->groupName() == group_id)
            {
                group = *i;
                break;
            }
        }
    }

    if (!group)
    {
        group = new LXQtTaskGroup(group_id, window, this);
        connect(group, SIGNAL(groupBecomeEmpty(QString)), this, SLOT(groupBecomeEmptySlot()));
        connect(group, SIGNAL(visibilityChanged(bool)), this, SLOT(refreshPlaceholderVisibility()));
        connect(group, &LXQtTaskGroup::popupShown, this, &LXQtTaskBar::popupShown);
        connect(group, &LXQtTaskButton::dragging, this, [this] (QObject * dragSource, QPoint const & pos) {
            buttonMove(qobject_cast<LXQtTaskGroup *>(sender()), qobject_cast<LXQtTaskGroup *>(dragSource), pos);
        });
        mLayout->addWidget(group);
        group->setToolButtonsStyle(mButtonStyle);

        if (mUngroupedNextToExisting)
        {
            const QString window_class = QString::fromUtf8(KWindowInfo(window, NET::Properties(), NET::WM2WindowClass).windowClassClass());
            int src_index = mLayout->count() - 1;
            int dst_index = src_index;
            for (int i = mLayout->count() - 2; 0 <= i; --i)
            {
                LXQtTaskGroup * current_group = qobject_cast<LXQtTaskGroup*>(mLayout->itemAt(i)->widget());
                if (nullptr != current_group)
                {
                    const QString current_class = QString::fromUtf8(KWindowInfo((current_group->groupName()).toUInt(), NET::Properties(), NET::WM2WindowClass).windowClassClass());
                    if(current_class == window_class)
                    {
                        dst_index = i + 1;
                        break;
                    }
                }
            }

            if (dst_index != src_index)
            {
                mLayout->moveItem(src_index, dst_index, false);
            }
        }
    }
    mKnownWindows[window] = group;
    group->addWindow(window);
}

/************************************************

 ************************************************/
auto LXQtTaskBar::removeWindow(windowMap_t::iterator pos) -> windowMap_t::iterator
{
    WId const window = pos.key();
    LXQtTaskGroup * const group = *pos;
    auto ret = mKnownWindows.erase(pos);
    group->onWindowRemoved(window);
    return ret;
}

/************************************************

 ************************************************/
void LXQtTaskBar::refreshTaskList()
{
    QList<WId> new_list;
    // Just add new windows to groups, deleting is up to the groups
    const auto wnds = KWindowSystem::stackingOrder();
    for (auto const wnd: wnds)
    {
        if (acceptWindow(wnd))
        {
            new_list << wnd;
            addWindow(wnd);
        }
    }

    //emulate windowRemoved if known window not reported by KWindowSystem
    for (auto i = mKnownWindows.begin(), i_e = mKnownWindows.end(); i != i_e; )
    {
        if (0 > new_list.indexOf(i.key()))
        {
            i = removeWindow(i);
        } else
            ++i;
    }

    refreshPlaceholderVisibility();
}

/************************************************

 ************************************************/
void LXQtTaskBar::onWindowChanged(WId window, NET::Properties prop, NET::Properties2 prop2)
{
    auto i = mKnownWindows.find(window);
    if (mKnownWindows.end() != i)
    {
        if (!(*i)->onWindowChanged(window, prop, prop2) && acceptWindow(window))
        { // window is removed from a group because of class change, so we should add it again
            addWindow(window);
        }
    }
}

void LXQtTaskBar::onWindowAdded(WId window)
{
    auto const pos = mKnownWindows.find(window);
    if (mKnownWindows.end() == pos && acceptWindow(window))
        addWindow(window);
}

/************************************************

 ************************************************/
void LXQtTaskBar::onWindowRemoved(WId window)
{
    auto const pos = mKnownWindows.find(window);
    if (mKnownWindows.end() != pos)
    {
        removeWindow(pos);
    }
}

/************************************************

 ************************************************/
void LXQtTaskBar::refreshButtonRotation()
{
    bool autoRotate = mAutoRotate && (mButtonStyle != Qt::ToolButtonIconOnly);

    ILXQtPanel::Position panelPosition = mPlugin->panel()->position();
    emit buttonRotationRefreshed(autoRotate, panelPosition);
}

/************************************************

 ************************************************/
void LXQtTaskBar::refreshPlaceholderVisibility()
{
    // if no visible group button show placeholder widget
    bool haveVisibleWindow = false;
    for (auto i = mKnownWindows.cbegin(), i_e = mKnownWindows.cend(); i_e != i; ++i)
    {
        if ((*i)->isVisible())
        {
            haveVisibleWindow = true;
            break;
        }
    }
    mPlaceHolder->setVisible(!haveVisibleWindow);
    if (haveVisibleWindow)
        mPlaceHolder->setFixedSize(0, 0);
    else
    {
        mPlaceHolder->setMinimumSize(1, 1);
        mPlaceHolder->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }

}

/************************************************

 ************************************************/
void LXQtTaskBar::setButtonStyle(Qt::ToolButtonStyle buttonStyle)
{
    const Qt::ToolButtonStyle old_style = mButtonStyle;
    mButtonStyle = buttonStyle;
    if (old_style != mButtonStyle)
        emit buttonStyleRefreshed(mButtonStyle);
}

/************************************************

 ************************************************/
void LXQtTaskBar::settingsChanged()
{
    bool groupingEnabledOld = mGroupingEnabled;
    bool ungroupedNextToExistingOld = mUngroupedNextToExisting;
    bool showOnlyOneDesktopTasksOld = mShowOnlyOneDesktopTasks;
    const int showDesktopNumOld = mShowDesktopNum;
    bool showOnlyCurrentScreenTasksOld = mShowOnlyCurrentScreenTasks;
    bool showOnlyMinimizedTasksOld = mShowOnlyMinimizedTasks;
    const bool iconByClassOld = mIconByClass;

    mButtonWidth = mPlugin->settings()->value(QStringLiteral("buttonWidth"), 400).toInt();
    mButtonHeight = mPlugin->settings()->value(QStringLiteral("buttonHeight"), 100).toInt();
    QString s = mPlugin->settings()->value(QStringLiteral("buttonStyle")).toString().toUpper();

    if (s == QStringLiteral("ICON"))
        setButtonStyle(Qt::ToolButtonIconOnly);
    else if (s == QStringLiteral("TEXT"))
        setButtonStyle(Qt::ToolButtonTextOnly);
    else
        setButtonStyle(Qt::ToolButtonTextBesideIcon);

    mShowOnlyOneDesktopTasks = mPlugin->settings()->value(QStringLiteral("showOnlyOneDesktopTasks"), mShowOnlyOneDesktopTasks).toBool();
    mShowDesktopNum = mPlugin->settings()->value(QStringLiteral("showDesktopNum"), mShowDesktopNum).toInt();
    mShowOnlyCurrentScreenTasks = mPlugin->settings()->value(QStringLiteral("showOnlyCurrentScreenTasks"), mShowOnlyCurrentScreenTasks).toBool();
    mShowOnlyMinimizedTasks = mPlugin->settings()->value(QStringLiteral("showOnlyMinimizedTasks"), mShowOnlyMinimizedTasks).toBool();
    mAutoRotate = mPlugin->settings()->value(QStringLiteral("autoRotate"), true).toBool();
    mCloseOnMiddleClick = mPlugin->settings()->value(QStringLiteral("closeOnMiddleClick"), true).toBool();
    mRaiseOnCurrentDesktop = mPlugin->settings()->value(QStringLiteral("raiseOnCurrentDesktop"), false).toBool();
    mGroupingEnabled = mPlugin->settings()->value(QStringLiteral("groupingEnabled"),true).toBool();
    mShowGroupOnHover = mPlugin->settings()->value(QStringLiteral("showGroupOnHover"),true).toBool();
    mUngroupedNextToExisting = mPlugin->settings()->value(QStringLiteral("ungroupedNextToExisting"),false).toBool();
    mIconByClass = mPlugin->settings()->value(QStringLiteral("iconByClass"), false).toBool();
    mWheelEventsAction = mPlugin->settings()->value(QStringLiteral("wheelEventsAction"), 1).toInt();
    mWheelDeltaThreshold = mPlugin->settings()->value(QStringLiteral("wheelDeltaThreshold"), 300).toInt();

    // Delete all groups if grouping or ungrouped next to existing feature toggled and start over
    if (groupingEnabledOld != mGroupingEnabled || ungroupedNextToExistingOld != mUngroupedNextToExisting)
    {
        for (int i = mLayout->count() - 1; 0 <= i; --i)
        {
            LXQtTaskGroup * group = qobject_cast<LXQtTaskGroup*>(mLayout->itemAt(i)->widget());
            if (nullptr != group)
            {
                mLayout->takeAt(i);
                group->deleteLater();
            }
        }
        mKnownWindows.clear();
    }

    if (showOnlyOneDesktopTasksOld != mShowOnlyOneDesktopTasks
            || (mShowOnlyOneDesktopTasks && showDesktopNumOld != mShowDesktopNum)
            || showOnlyCurrentScreenTasksOld != mShowOnlyCurrentScreenTasks
            || showOnlyMinimizedTasksOld != mShowOnlyMinimizedTasks
            )
        emit showOnlySettingChanged();
    if (iconByClassOld != mIconByClass)
        emit iconByClassChanged();

    refreshTaskList();
}

/************************************************

 ************************************************/
void LXQtTaskBar::realign()
{
    mLayout->setEnabled(false);
    refreshButtonRotation();

    ILXQtPanel *panel = mPlugin->panel();
    QSize maxSize = QSize(mButtonWidth, mButtonHeight);
    QSize minSize = QSize(0, 0);

    bool rotated = false;

    if (panel->isHorizontal())
    {
        mLayout->setRowCount(panel->lineCount());
        mLayout->setColumnCount(0);
    }
    else
    {
        mLayout->setRowCount(0);

        if (mButtonStyle == Qt::ToolButtonIconOnly)
        {
            // Vertical + Icons
            mLayout->setColumnCount(panel->lineCount());
        }
        else
        {
            rotated = mAutoRotate && (panel->position() == ILXQtPanel::PositionLeft || panel->position() == ILXQtPanel::PositionRight);

            // Vertical + Text
            if (rotated)
            {
                maxSize.rwidth()  = mButtonHeight;
                maxSize.rheight() = mButtonWidth;

                mLayout->setColumnCount(panel->lineCount());
            }
            else
            {
                mLayout->setColumnCount(1);
            }
        }
    }

    mLayout->setCellMinimumSize(minSize);
    mLayout->setCellMaximumSize(maxSize);
    mLayout->setDirection(rotated ? LXQt::GridLayout::TopToBottom : LXQt::GridLayout::LeftToRight);
    mLayout->setEnabled(true);

    //our placement on screen could have been changed
    emit showOnlySettingChanged();
    emit refreshIconGeometry();
}

/************************************************

 ************************************************/
void LXQtTaskBar::wheelEvent(QWheelEvent* event)
{
    // ignore wheel action unless user preference is "cycle windows"
    if (mWheelEventsAction != 1)
        return QFrame::wheelEvent(event);

    static int threshold = 0;

    QPoint angleDelta = event->angleDelta();
    Qt::Orientation orient = (qAbs(angleDelta.x()) > qAbs(angleDelta.y()) ? Qt::Horizontal : Qt::Vertical);
    int delta = (orient == Qt::Horizontal ? angleDelta.x() : angleDelta.y());

    threshold += abs(delta);
    if (threshold < mWheelDeltaThreshold)
        return QFrame::wheelEvent(event);
    else
        threshold = 0;

    int D = delta < 0 ? 1 : -1;

    // create temporary list of visible groups in the same order like on the layout
    QList<LXQtTaskGroup*> list;
    LXQtTaskGroup *group = nullptr;
    for (int i = 0; i < mLayout->count(); i++)
    {
        QWidget * o = mLayout->itemAt(i)->widget();
        LXQtTaskGroup * g = qobject_cast<LXQtTaskGroup *>(o);
        if (!g)
            continue;

        if (g->isVisible())
            list.append(g);
        if (g->isChecked())
            group = g;
    }

    if (list.isEmpty())
        return QFrame::wheelEvent(event);

    if (!group)
        group = list.at(0);

    LXQtTaskButton *button = nullptr;

    // switching between groups from temporary list in modulo addressing
    while (!button)
    {
        button = group->getNextPrevChildButton(D == 1, !(list.count() - 1));
        if (button)
            button->raiseApplication();
        int idx = (list.indexOf(group) + D + list.count()) % list.count();
        group = list.at(idx);
    }
    QFrame::wheelEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::resizeEvent(QResizeEvent* event)
{
    emit refreshIconGeometry();
    return QWidget::resizeEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::changeEvent(QEvent* event)
{
    // if current style is changed, reset the base style of the proxy style
    // so we can apply the new style correctly to task buttons.
    if(event->type() == QEvent::StyleChange)
        mStyle->setBaseStyle(nullptr);

    QFrame::changeEvent(event);
}

/************************************************

 ************************************************/
void LXQtTaskBar::registerShortcuts()
{
    // Register shortcuts to switch to the task
    // mPlaceHolder is always at position 0
    // tasks are at positions 1..10
    GlobalKeyShortcut::Action * gshortcut;
    QString path;
    QString description;
    for (int i = 1; i <= 10; ++i)
    {
        path = QStringLiteral("/panel/%1/task_%2").arg(mPlugin->settings()->group()).arg(i);
        description = tr("Activate task %1").arg(i);

        gshortcut = GlobalKeyShortcut::Client::instance()->addAction(QStringLiteral(), path, description, this);

        if (nullptr != gshortcut)
        {
            mKeys << gshortcut;
            connect(gshortcut, &GlobalKeyShortcut::Action::registrationFinished, this, &LXQtTaskBar::shortcutRegistered);
            connect(gshortcut, &GlobalKeyShortcut::Action::activated, mSignalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
            mSignalMapper->setMapping(gshortcut, i);
        }
    }
}

void LXQtTaskBar::shortcutRegistered()
{
    GlobalKeyShortcut::Action * const shortcut = qobject_cast<GlobalKeyShortcut::Action*>(sender());

    disconnect(shortcut, &GlobalKeyShortcut::Action::registrationFinished, this, &LXQtTaskBar::shortcutRegistered);

    const int i = mKeys.indexOf(shortcut);
    Q_ASSERT(-1 != i);

    if (shortcut->shortcut().isEmpty())
    {
        // Shortcuts come in order they were registered
        // starting from index 0
        const int key = (i + 1) % 10;
        shortcut->changeShortcut(QStringLiteral("Meta+%1").arg(key));
    }
}

void LXQtTaskBar::activateTask(int pos)
{
    for (int i = 1; i < mLayout->count(); ++i)
    {
        QWidget * o = mLayout->itemAt(i)->widget();
        LXQtTaskGroup * g = qobject_cast<LXQtTaskGroup *>(o);
        if (g && g->isVisible())
        {
            pos--;
            if (pos == 0)
            {
                g->raiseApplication();
                break;
            }
        }
    }
}
