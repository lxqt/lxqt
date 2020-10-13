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

#ifndef NOTIFYD_H
#define NOTIFYD_H

#include <QString>
#include <QObject>
#include <QVariantMap>
#include <QStringList>
#include <QPointer>
#include <QSystemTrayIcon>
#include <QMenu>

#include "notificationarea.h"
#include <LXQt/Settings>

/*
 * Class for interface org.freedesktop.Notifications
 */
class Notifyd: public QObject
{
    Q_OBJECT

public:
    explicit Notifyd(QObject* parent = nullptr);
    virtual ~Notifyd();

public slots:

    /*! External application requested to close one notification
     * \param id an id obtained by previous \c Notify() call
     */
    void CloseNotification(uint id);

    /*! List supported features as required by specification
     * \retval QStringList with features.
     */
    QStringList GetCapabilities();

    /*! Provide a server/deamon info about version etc.
     * \param vendor outbound string. It's filled with lxqt credits
     * \param version outbound string. lxqt DE version is used (from CMakeLists.txt)
     * \param spec_version outbound string. Currently used specification version.
     * \retval QString daemon binary name
     */
    QString GetServerInformation(QString& vendor, QString& version, QString& spec_version);

    /*! Daemon is requested to display the notification.
     * \retval uint an unique notification ID. It should be used for tracking action feedback.
     * \param app_name name of the calling application
     * \param replaces_id if it's provided (!= 0) the already posted notification is "replaced"
     *                    or updated with new content.
     * \param app_icon an icon to show. See specification for more info.
     * \param summary an overview text
     * \param body a notification body
     * \param actions User selectable action list. See specification for more info.
     * \param hints Notification hints. See specification for more info.
     * \param expire_timeout how long should be this notification displayed (-1 = server decides; 0 forever; >0timeout in milliseconds)
     * \param noSave never save or remember this notification; always assume it is closed by user
     */
    uint Notify(const QString& app_name,
                uint replaces_id,
                const QString& app_icon,
                const QString& summary,
                const QString& body,
                const QStringList& actions,
                const QVariantMap& hints,
                int expire_timeout,
                bool noSave = false);

signals:
    // signals for DBUS API specs - going outside

    /*! Inform the external application that user chose one of provided action
     * \param in0 a notification ID (obtained from \c Notify)
     * \param in1 a selected action key from the (key - display value) pair
     */
    void ActionInvoked(uint in0, const QString& in1);
    /*! Inform the external application that notification has been closed.
     * \param in0 a notification ID (obtained from \c Notify)
     * \param in1 a closing reason. See specification for more info.
     *
     *
        1 - The notification expired.
        2 - The notification was dismissed by the user.
        3 - The notification was closed by a call to CloseNotification.
        4 - Undefined/reserved reasons.
     */
    void NotificationClosed(uint in0, uint in1);

    // signals for our implementation - internal usage

    /*! Promote the external request for notification closing into the display engine.
     * \param id an notification ID (obtained from \c Notify)
     * \param reason a reason for closing code. See specification for more info.
     */
    void notificationClosed(uint id, uint reason);

    /*! Promote the external request for notification displaying into the engine.
     * Parameters are the same as in \c Notify() slot.
     */
    void notificationAdded(uint id, const QString &application, const QString &title,
                           const QString &description, const QString &icon,
                           int timeout, const QStringList& actions, const QVariantMap& hints,
                           bool noSave);

private:
    uint mId;
    NotificationArea *m_area;
    int m_serverTimeout;

    LXQt::Settings *m_settings;

    QPointer<QSystemTrayIcon> m_trayIcon;
    QPointer<QMenu> m_trayMenu;
    int m_trayChecker;

    void createTrayIcon();

private slots:
    void reloadSettings();
    void checkTray();
    void addToUnattendedList(uint id, uint reason, const QString &date);
    void restoreUnattended();
};

#endif // NOTIFYD_H
