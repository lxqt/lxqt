/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
 * Authors:
 *   Alec Moskvin <alecm@gmx.com>
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

#include "notifyd.h"

#include <QProcess>
#include <QDebug>


/*
 * Implementation of class Notifyd
 */

Notifyd::Notifyd(QObject* parent)
    : QObject(parent),
      mId(0),
      m_trayChecker(0)
{
    m_area = new NotificationArea();
    m_settings = new LXQt::Settings(QSL("notifications"));
    reloadSettings();

    connect(this, &Notifyd::notificationAdded,
            m_area->layout(), &NotificationLayout::addNotification);
    connect(this, &Notifyd::notificationClosed,
            m_area->layout(), &NotificationLayout::removeNotification);
    // feedback for original caller
    connect(m_area->layout(), &NotificationLayout::notificationClosed,
            [this] (uint id, uint reason, const QString& /*date*/) {
                emit NotificationClosed(id, reason);
            });
    connect(m_area->layout(), &NotificationLayout::notificationClosed,
            this, &Notifyd::addToUnattendedList);
    connect(m_area->layout(), &NotificationLayout::actionInvoked,
            this, &Notifyd::ActionInvoked);

    connect(m_settings, &LXQt::Settings::settingsChanged,
            this, &Notifyd::reloadSettings);

}

Notifyd::~Notifyd()
{
    m_trayMenu->deleteLater();
    m_trayIcon->deleteLater();
    m_area->deleteLater();
}

void Notifyd::CloseNotification(uint id)
{
    emit notificationClosed(id, 3);
}

QStringList Notifyd::GetCapabilities()
{
    QStringList caps;
    caps
         << QSL("actions")
      // << "action-icons"
         << QSL("body")
         << QSL("body-hyperlinks")
         << QSL("body-images")
         << QSL("body-markup")
      // << "icon-multi"
      // << "icon-static"
         << QSL("persistence")
      // << "sound"
      ;
    return caps;
}

QString Notifyd::GetServerInformation(QString& vendor,
                                      QString& version,
                                      QString& spec_version)
{
    spec_version = QSL("1.2");
    version = QSL(LXQT_VERSION);
    vendor = QSL("lxqt.org");
    return QSL("lxqt-notificationd");
}

uint Notifyd::Notify(const QString& app_name,
                     uint replaces_id,
                     const QString& app_icon,
                     const QString& summary,
                     const QString& body,
                     const QStringList& actions,
                     const QVariantMap& hints,
                     int expire_timeout,
                     bool noSave)
{
    uint ret;
    if (replaces_id == 0)
    {
        mId++;
        ret = mId;
    }
    else
        ret = replaces_id;
#if 0
    qDebug() << QString("Notify(\n"
                        "  app_name = %1\n"
                        "  replaces_id = %2\n"
                        "  app_icon = %3\n"
                        "  summary = %4\n"
                        "  body = %5\n"
                        ).arg(app_name, QString::number(replaces_id), app_icon, summary, body)
                     << "  actions =" << actions << endl
                     << "  hints =" << hints << endl
             << QString("  expire_timeout = %1\n) => %2").arg(QString::number(expire_timeout), QString::number(mId));
#endif

    // handling the "server decides" timeout
    if (expire_timeout == -1) {
        expire_timeout = m_serverTimeout;
        expire_timeout *= 1000;
    }

    emit notificationAdded(ret, app_name, summary, body, app_icon, expire_timeout, actions, hints, noSave);

    return ret;
}

void Notifyd::reloadSettings()
{
    m_serverTimeout = m_settings->value(QSL("server_decides"), 10).toInt();
    int maxNum = m_settings->value(QSL("unattendedMaxNum"), 10).toInt();
    m_area->setSettings(
            m_settings->value(QSL("placement"), QSL("bottom-right")).toString().toLower(),
            m_settings->value(QSL("width"), 300).toInt(),
            m_settings->value(QSL("spacing"), 6).toInt(),
            maxNum,
            m_settings->value(QSL("screenWithMouse"),false).toBool(),
            m_settings->value(QSL("blackList")).toStringList());

    if (maxNum > 0 && m_trayIcon.isNull())
    { // create the tray icon
        if (QSystemTrayIcon::isSystemTrayAvailable())
            createTrayIcon();
        else // check the tray's presence every 5 seconds (see checkTray)
        {
            QTimer *trayTimer = new QTimer(this);
            trayTimer->setSingleShot(true);
            trayTimer->setInterval(5000);
            connect(trayTimer, &QTimer::timeout, this, &Notifyd::checkTray);
            trayTimer->start();
            ++ m_trayChecker;
        }
    }
    else if (maxNum == 0 && !m_trayIcon.isNull())
    { // remove the tray icon
        m_trayMenu->deleteLater();
        m_trayIcon->deleteLater();
    }
}

// Creates the tray icon and populates its context menu.
void Notifyd::createTrayIcon()
{
    if (m_trayMenu.isNull())
        m_trayMenu = new QMenu();
    else
        m_trayMenu->clear();

    if (m_trayIcon.isNull())
    {
        m_trayIcon = new QSystemTrayIcon(QIcon::fromTheme(QSL("preferences-desktop-notification")), this);
        /* show the menu also on left clicking */
        connect(m_trayIcon, &QSystemTrayIcon::activated, [this] (QSystemTrayIcon::ActivationReason r) {
            if (r == QSystemTrayIcon::Trigger && !m_trayMenu.isNull())
                m_trayMenu->exec(QCursor::pos());
        });
    }

    QSettings list(m_area->layout()->cacheFile(), QSettings::IniFormat);
    QStringList dates = list.childGroups();
    if (!dates.isEmpty())
        dates.sort();

    QAction *action = nullptr;
    // add items for notification, starting from the oldest one and from bottom to top
    for (const QString &date : qAsConst(dates))
    {
        list.beginGroup(date);
        // "DATE_AND_TIME - APP: SUMMARY"
        QString txt = QDateTime::fromString(date, m_area->layout()->cacheDateFormat())
                        .toString(Qt::SystemLocaleShortDate) // for human readability
                        + QL1S(" - ") + list.value(QL1S("Application")).toString() + QL1S(": ")
                        + list.value(QL1S("Summary")).toString();
        list.endGroup();
        QAction *thisAction = new QAction(txt, m_trayMenu);
        thisAction->setData(date);
        m_trayMenu->insertAction(action, thisAction);
        action = thisAction;
        connect(thisAction, &QAction::triggered, this, &Notifyd::restoreUnattended);
    }

    // add a separator
    m_trayMenu->addSeparator();

    // "Clear All"
    action = m_trayMenu->addAction(QIcon::fromTheme(QSL("edit-clear")), tr("Clear All"));
    connect(action, &QAction::triggered, m_trayMenu, [this] {
        m_trayIcon->setVisible(false);
        m_trayMenu->deleteLater();
        QSettings(m_area->layout()->cacheFile(), QSettings::IniFormat).clear();
    });

    // "Options"
    action = m_trayMenu->addAction(QIcon::fromTheme(QSL("preferences-system")), tr("Options"));
    connect(action, &QAction::triggered, m_trayMenu, [] {
        QProcess::startDetached(QSL("lxqt-config-notificationd"), QStringList());
    });

    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setVisible(!dates.isEmpty());
    m_trayIcon->setToolTip(tr("%n Unattended Notification(s)", "",
                                m_trayMenu->actions().size() - 3));
            
}

// NOTE: Contrary to what Qt doc implies, if the tray icon is created before the tray area is available,
// it will never be shown. This function checks the tray's presence every 5 seconds for one minute.
void Notifyd::checkTray()
{
    if (QTimer *trayTimer = qobject_cast<QTimer*>(sender()))
    {
        if (QSystemTrayIcon::isSystemTrayAvailable())
        {
            trayTimer->deleteLater();
            createTrayIcon();
        }
        else if (m_trayChecker < 12)
        {
            trayTimer->start();
            ++ m_trayChecker;
        }
        else
            trayTimer->deleteLater();
    }
}

void Notifyd::addToUnattendedList(uint /*id*/, uint reason, const QString &date)
{
    // process the notifications if it's unattended and not blacklisted
    if (reason == 1 && !date.isEmpty() && m_area->layout()->getUnattendedMaxNum() > 0)
    {
        if (m_trayIcon.isNull() || m_trayMenu.isNull())
        {
            if (QSystemTrayIcon::isSystemTrayAvailable())
                createTrayIcon();
        }
        else
        {
            // Add it to the top of the current tray menu, removing the oldest items if the list is full.
            // A separator, "Clear All" and "Options" exist after all items.
            QList<QAction*> actions = m_trayMenu->actions();
            while (actions.size() >= m_area->layout()->getUnattendedMaxNum() + 3)
            {
                m_trayMenu->removeAction(actions.at(actions.size() - 4));
                delete actions.takeAt(actions.size() - 4);
            }
            QSettings list(m_area->layout()->cacheFile(), QSettings::IniFormat);
            list.beginGroup(date);
            QString txt = QDateTime::fromString(date, m_area->layout()->cacheDateFormat())
                            .toString(Qt::SystemLocaleShortDate) // for human readability
                            + QL1S(" - ") + list.value(QL1S("Application")).toString() + QL1S(": ")
                            + list.value(QL1S("Summary")).toString();
            list.endGroup();
            QAction *action = new QAction(txt, m_trayMenu);
            action->setData(date);
            m_trayMenu->insertAction(actions.isEmpty() ? nullptr : actions.first(), action);
            connect(action, &QAction::triggered, this, &Notifyd::restoreUnattended);
            m_trayIcon->setVisible(true);
            m_trayIcon->setToolTip(tr("%n Unattended Notification(s)", "",
                                       m_trayMenu->actions().size() - 3));
        }
    }
}

void Notifyd::restoreUnattended()
{
    if (QAction *action = qobject_cast<QAction*>(sender()))
    {
        const QString date = action->data().toString();
        m_trayMenu->removeAction(action);
        delete action;
        if (m_trayMenu->actions().size() == 3) // separator, "Clear All" and "Options" exist
        {
            m_trayIcon->setVisible(false);
            // NOTE: If the menu isn't deleted here, it won't be shown later (a Qt bug?).
            m_trayMenu->deleteLater();
        }
        else
            m_trayIcon->setToolTip(tr("%n Unattended Notification(s)", "",
                                       m_trayMenu->actions().size() - 3));
        QSettings list(m_area->layout()->cacheFile(), QSettings::IniFormat);
        if (list.childGroups().contains(date))
        {
            list.beginGroup(date);
            Notify(list.value(QL1S("Application")).toString(),
                0,
                list.value(QL1S("Icon")).toString(),
                list.value(QL1S("Summary")).toString(),
                list.value(QL1S("Body")).toString(),
                list.value(QL1S("Actions")).toStringList(),
                list.value(QL1S("Hints")).toMap(),
                list.value(QL1S("TimeOut")).toInt(),
                true); // don't save this notification again; see this as a user interaction
            list.endGroup();
            list.remove(date);
        }
    }
}
