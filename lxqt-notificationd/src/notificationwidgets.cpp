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

#include <LXQt/Globals>

#include <QComboBox>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QPushButton>
#include <QLabel>

#include "notificationwidgets.h"

#include <QtDebug>


NotificationActionsWidget::NotificationActionsWidget(const QStringList& actions, QWidget *parent)
    : QWidget(parent)
{
    for (int i = 0; i < actions.count(); i += 2)
    {
        QString key = actions[i];
        QString value;

        if (i == actions.count() - 1)
        {
            value = key;
            qWarning() << "Odd number of elements in action list. Last action will use key as text (" << key << ")";
        } else {
            value = actions[i + 1];
        }

        if (key == QL1S("default"))
            m_defaultAction = key;

        m_actions.append({key, value});
    }

    // if there is only one action let's use it as the default one
    if (m_actions.count() == 1)
        m_defaultAction = m_actions[0].first;
}


NotificationActionsButtonsWidget::NotificationActionsButtonsWidget(const QStringList& actions, QWidget *parent)
    : NotificationActionsWidget(actions, parent)
{
    QHBoxLayout *l = new QHBoxLayout();
    setLayout(l);

    QButtonGroup *group = new QButtonGroup(this);

    for (const auto &action : qAsConst(m_actions))
    {
        QPushButton *b = new QPushButton(action.second, this);
        b->setObjectName(action.first);
        l->addWidget(b);
        group->addButton(b);

        if (action.first == m_defaultAction)
            b->setFocus(Qt::OtherFocusReason);
    }
    connect(group, static_cast<void (QButtonGroup::*)(QAbstractButton*)>(&QButtonGroup::buttonClicked),
            this, &NotificationActionsButtonsWidget::actionButtonActivated);
}

void NotificationActionsButtonsWidget::actionButtonActivated(QAbstractButton* button)
{
    emit actionTriggered(button->objectName());
}


NotificationActionsComboWidget::NotificationActionsComboWidget(const QStringList& actions, QWidget *parent)
    : NotificationActionsWidget(actions, parent)
{
    QHBoxLayout *l = new QHBoxLayout();
    setLayout(l);

    l->addWidget(new QLabel(tr("Actions:"), this));
    m_comboBox = new QComboBox(this);
    int currentIndex = -1;

    for (int i = 0; i < m_actions.count(); ++i)
    {
        auto const & action = m_actions[i];

        m_comboBox->addItem(action.second, action.first);
        if (action.first == m_defaultAction)
        {
            currentIndex = i;
        }
    }
    l->addWidget(m_comboBox);

    if (currentIndex != -1)
        m_comboBox->setCurrentIndex(currentIndex);

    QPushButton *b = new QPushButton(tr("OK"), this);
    l->addWidget(b);
    connect(b, &QPushButton::clicked,
            this, &NotificationActionsComboWidget::actionComboBoxActivated);
}

void NotificationActionsComboWidget::actionComboBoxActivated()
{
    if (!m_comboBox)
        return;
    int ix = m_comboBox->currentIndex();
    if (ix == -1)
        return;
    emit actionTriggered(m_actions[ix].first);
}
