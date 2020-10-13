/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2012 Razor team
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

#ifndef NotificationLayout_H
#define NotificationLayout_H

#include "notification.h"


class NotificationLayout : public QWidget
{
    Q_OBJECT
public:
    explicit NotificationLayout(QWidget *parent);

    /*! Set various properties for self and child \c Notification instances.
     * \param space a layout spacing
     * \param width new width for notifications
     */
    void setSizes(int space, int width);

    int getUnattendedMaxNum() const {
        return m_unattendedMaxNum;
    }
    void setUnattendedMaxNum(int num) {
        m_unattendedMaxNum = num;
    }

    void setBlackList(const QStringList &l) {
        m_blackList = l;
    }

    QString cacheFile() const {
        return m_cacheFile;
    }
    QString cacheDateFormat() const {
        return m_cacheDateFormat;
    }

signals:
    //! All \c Notification instances are closed
    void allNotificationsClosed();
    //! At least one \c Notification instance is available and needs to be shown
    void notificationAvailable();
    //! Height of this widget changed so parent's \c NotificationArea needs to change its size too.
    void heightChanged(int);

    /*! Promote the internal change of notification closing into the \c Notifyd
     * \param id an notification ID (obtained from \c Notify)
     * \param reason a reason for closing code. See specification for more info.
     * \param date the exact moment of closing.
     */
    void notificationClosed(uint id, uint reason, const QString &date);

    /*! Inform the external application that user chose one of provided action via the \c Notifyd
     * \param in0 a notification ID (obtained from \c Notify)
     * \param in1 a selected action key from the (key - display value) pair
     */
    void actionInvoked(uint id, const QString &actionKey);

public slots:
    /*! Add new notification
     * See \c Notifyd::Notify() for params meanings.
     */
    void addNotification(uint id, const QString &application,
                         const QString &summary, const QString &body,
                         const QString &icon, int timeout,
                         const QStringList& actions, const QVariantMap& hints,
                         bool noSave = false);

    /*! Notification id should be removed because of reason
     */
    void removeNotification(uint id, uint reason);

private:
    QHash<uint, Notification*> m_notifications;
    QVBoxLayout *m_layout;
    int m_unattendedMaxNum;
    QStringList m_blackList;
    QString m_cacheFile;
    QString m_cacheDateFormat;

    /*! Calculate required height based on height of each \c Notification
     * in the m_notifications map.
     * Also heightChanged() is emitted here.
     */
    void checkHeight();

private slots:
    /*! \c Notification's timer timeouted, so closing the notifiaction
     */
    void removeNotificationTimeout();

    /*! \c User cancelled the notifiation manually
     */
    void removeNotificationUser();

    /*! User clicked on one of actions (if provioded).
     * \param actionKey an action's key (not the display value)
     */
    void notificationActionCalled(const QString &actionKey);
};

#endif // NotificationLayout_H
