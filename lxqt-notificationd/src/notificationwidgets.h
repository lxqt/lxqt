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

#ifndef NOTIFICATIONWIDGETS_H
#define NOTIFICATIONWIDGETS_H

#include <QHash>
#include <QWidget>
#include <QAbstractButton>

class QComboBox;


/*! A helper widgets for actions handling.
 * See specification for information what actions are.
 *
 * Let's be a little bit tricky here. Let's allow only few
 * buttons in the layout. We will use a combobox if there
 * are more actions. I think it's more user friendly.
 *
 * If there are only few actions the layout with buttons is used.
 * If there are more actions the combo box with confirm button is created.
 */
class NotificationActionsWidget : public QWidget
{
    Q_OBJECT

public:
    NotificationActionsWidget(const QStringList& actions, QWidget *parent);

    //! Notification holds exactly one action or at least one action is marked as "default"
    bool hasDefaultAction() { return !m_defaultAction.isEmpty(); }
    //! The key for default action
    QString defaultAction() { return m_defaultAction; }

signals:
    /*! User clicks/chose an actio
     * \param actionKey a key of selected action
     */
    void actionTriggered(const QString &actionKey);

protected:
    QString m_defaultAction;
    QList<QPair<QString/*action key*/, QString/*action value*/>> m_actions;
};

class NotificationActionsButtonsWidget : public NotificationActionsWidget
{
    Q_OBJECT

public:
    /*! Create new widget.
     * \param actions a list of actions in form: (key1, display1, key2, display2, ..., keyN, displayN)
     */
    NotificationActionsButtonsWidget(const QStringList& actions, QWidget *parent);
private slots:
    void actionButtonActivated(QAbstractButton* button);
};

class NotificationActionsComboWidget : public NotificationActionsWidget
{
    Q_OBJECT

public:
    /*! Create new widget.
     * \param actions a list of actions in form: (key1, display1, key2, display2, ..., keyN, displayN)
     */
    NotificationActionsComboWidget(const QStringList& actions, QWidget *parent);

private:
    QComboBox *m_comboBox;

private slots:
    void actionComboBoxActivated();
};


#endif
