/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *  Balázs Béla <balazsbela[at]gmail.com>
 *  Paulo Lieuthier <paulolieuthier@gmail.com>
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

#include "statusnotifierwidget.h"
#include <QApplication>
#include <QDebug>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QDBusConnectionInterface>
#include "../panel/pluginsettings.h"
#include "../panel/ilxqtpanelplugin.h"

StatusNotifierWidget::StatusNotifierWidget(ILXQtPanelPlugin *plugin, QWidget *parent) :
    QWidget(parent),
    mPlugin(plugin),
    mAttentionPeriod(5),
    mForceVisible(false)
{
    setLayout(new LXQt::GridLayout(this));

    // The button that shows all hidden items:
    mShowBtn = new QToolButton(this);
    mShowBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mShowBtn->setAutoRaise(true);
    mShowBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    mShowBtn->setText(QStringLiteral("+"));
    layout()->addWidget(mShowBtn);
    mShowBtn->hide();
    connect(mShowBtn, &QAbstractButton::clicked, [this] {
        if (mForceVisible)
            return; // all items are visible; nothing to do
        mShowBtn->hide();
        mHideTimer.stop();
        mForceVisible = true;
        const auto allButtons = findChildren<StatusNotifierButton *>(QString(), Qt::FindDirectChildrenOnly);
        for (const auto &btn : allButtons)
            btn->show();
    });

    settingsChanged();

    // The timer that hides (auto-)hidden items after 2 seconds:
    mHideTimer.setSingleShot(true);
    mHideTimer.setInterval(2000);
    connect(&mHideTimer, &QTimer::timeout, this, [this] {
        mShowBtn->show();
        mForceVisible = false;
        const auto allButtons = findChildren<StatusNotifierButton *>(QString(), Qt::FindDirectChildrenOnly);
        for (const auto &btn : allButtons)
        {
            if (btn->hasAttention()
                || (!mAutoHideList.contains(btn->title())
                     && !mHideList.contains(btn->title())))
            {
                continue;
            }
            btn->hide();
        }
    });

    QFutureWatcher<StatusNotifierWatcher *> * future_watcher = new QFutureWatcher<StatusNotifierWatcher *>;
    connect(future_watcher, &QFutureWatcher<StatusNotifierWatcher *>::finished, this, [this, future_watcher]
        {
            mWatcher = future_watcher->future().result();

            connect(mWatcher, &StatusNotifierWatcher::StatusNotifierItemRegistered,
                    this, &StatusNotifierWidget::itemAdded);
            connect(mWatcher, &StatusNotifierWatcher::StatusNotifierItemUnregistered,
                    this, &StatusNotifierWidget::itemRemoved);

            qDebug() << mWatcher->RegisteredStatusNotifierItems();

            future_watcher->deleteLater();
        });

    QFuture<StatusNotifierWatcher *> future = QtConcurrent::run([]
        {
            QString dbusName = QStringLiteral("org.kde.StatusNotifierHost-%1-%2").arg(QApplication::applicationPid()).arg(1);
            if (QDBusConnectionInterface::ServiceNotRegistered == QDBusConnection::sessionBus().interface()->registerService(dbusName, QDBusConnectionInterface::DontQueueService))
                qDebug() << "unable to register service for " << dbusName;

            StatusNotifierWatcher * watcher = new StatusNotifierWatcher;
            watcher->RegisterStatusNotifierHost(dbusName);
            watcher->moveToThread(QApplication::instance()->thread());
            return watcher;
        });

    future_watcher->setFuture(future);

    realign();
}

StatusNotifierWidget::~StatusNotifierWidget()
{
    delete mWatcher;
}

void StatusNotifierWidget::leaveEvent(QEvent * /*event*/)
{
    if (mForceVisible)
        mHideTimer.start();
}

void StatusNotifierWidget::enterEvent(QEvent * /*event*/)
{
    mHideTimer.stop();
}

void StatusNotifierWidget::itemAdded(QString serviceAndPath)
{
    int slash = serviceAndPath.indexOf(QLatin1Char('/'));
    QString serv = serviceAndPath.left(slash);
    QString path = serviceAndPath.mid(slash);
    StatusNotifierButton *button = new StatusNotifierButton(serv, path, mPlugin, this);

    mServices.insert(serviceAndPath, button);
    layout()->addWidget(button);
    button->show();

    // show/hide the added item appropriately and show mShowBtn if needed
    connect(button, &StatusNotifierButton::titleFound, [this, button] (const QString &title) {
        mItemTitles << title;
        if (mAutoHideList.contains(title))
        {
            if (!mForceVisible)
                mShowBtn->show();
            button->setAutoHide(true, mAttentionPeriod, mForceVisible);
        }
        else if (mHideList.contains(title))
        {
            button->setAutoHide(false);
            if (!mForceVisible)
            {
                mShowBtn->show();
                button->hide();
            }
        }
    });
    // show/hide mShowBtn if needed whenever an item gets or loses attention
    connect(button, &StatusNotifierButton::attentionChanged, [this, button] {
        if (button->hasAttention())
        {
            if (mShowBtn->isVisible() || mForceVisible)
            {
                const auto allButtons = findChildren<StatusNotifierButton *>(QString(), Qt::FindDirectChildrenOnly);
                for (const auto &btn : allButtons)
                {
                    if (!btn->isVisible()
                        // or shown only because mShowBtn was clicked
                        || (mForceVisible && !btn->hasAttention()
                            && (mAutoHideList.contains(btn->title())
                                || mHideList.contains(btn->title()))))
                    {
                        return;
                    }
                }
                // there is no item in the hiding list and all auto-hiding items have attention;
                // so, mShowBtn has no job
                mHideTimer.stop();
                mForceVisible = false;
                mShowBtn->hide();
            }
        }
        else // the auto-hiding item lost attention
        {
            if (!mForceVisible)
                mShowBtn->show();
        }
    });
}

void StatusNotifierWidget::itemRemoved(const QString &serviceAndPath)
{
    StatusNotifierButton *button = mServices.value(serviceAndPath, nullptr);
    if (button)
    {
        mItemTitles.removeOne(button->title());
        if (mShowBtn->isVisible() || mForceVisible)
        { // hide mShowBtn if no (auto-)hidden item remains
            bool showBtn = false;
            for (const auto &name : qAsConst(mItemTitles))
            {
                if (mAutoHideList.contains(name) || mHideList.contains(name))
                {
                    showBtn = true;
                    break;
                }
            }
            if (!showBtn)
            {
                mHideTimer.stop();
                mForceVisible = false;
                mShowBtn->hide();
            }
        }
        button->deleteLater();
        layout()->removeWidget(button);
        mServices.remove(serviceAndPath);
    }
}

void StatusNotifierWidget::settingsChanged()
{
    mAttentionPeriod = mPlugin->settings()->value(QStringLiteral("attentionPeriod"), 5).toInt();
    mAutoHideList = mPlugin->settings()->value(QStringLiteral("autoHideList")).toStringList();
    mHideList = mPlugin->settings()->value(QStringLiteral("hideList")).toStringList();

    // show/hide items as well as showBtn appropriately
    const auto allButtons = findChildren<StatusNotifierButton *>(QString(), Qt::FindDirectChildrenOnly);
    bool showBtn = false;
    for (const auto &btn : allButtons)
    {
        if (mAutoHideList.contains(btn->title()))
        {
            btn->setAutoHide(true, mAttentionPeriod);
            if (!btn->isVisible()
                // or shown only because mShowBtn was clicked
                || !btn->hasAttention())
            {
                showBtn = true;
            }
        }
        else if (mHideList.contains(btn->title()))
        {
            showBtn = true;
            btn->setAutoHide(false);
            btn->hide();
        }
        else
        {
            btn->setAutoHide(false);
            btn->show(); // may have been in mHideList before
        }
    }
    if (!showBtn)
    {
        mHideTimer.stop();
        mForceVisible = false;
        mShowBtn->hide();
    }
    else if (!mForceVisible)
        mShowBtn->show();
}

void StatusNotifierWidget::realign()
{
    LXQt::GridLayout *layout = qobject_cast<LXQt::GridLayout*>(this->layout());
    layout->setEnabled(false);

    ILXQtPanel *panel = mPlugin->panel();
    if (panel->isHorizontal())
    {
        layout->setRowCount(panel->lineCount());
        layout->setColumnCount(0);
    }
    else
    {
        layout->setColumnCount(panel->lineCount());
        layout->setRowCount(0);
    }

    layout->setEnabled(true);
}

QStringList StatusNotifierWidget::itemTitles() const
{
    QStringList names = mItemTitles;
    names.removeDuplicates();
    return names;
}
