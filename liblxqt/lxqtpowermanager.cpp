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

#include "lxqtpowermanager.h"
#include "lxqtpower/lxqtpower.h"
#include <QDBusInterface>
#include <QMessageBox>
#include <QApplication>
#include <QtDebug>
#include <QScreen>
#include <QFile>
#include "lxqttranslator.h"
#include "lxqtglobals.h"
#include "lxqtsettings.h"
#include <XdgIcon>

namespace LXQt {

class LXQT_API MessageBox: public QMessageBox
{
    Q_DECLARE_TR_FUNCTIONS(LXQt::MessageBox)

public:
    explicit MessageBox(QWidget *parent = nullptr): QMessageBox(parent) {}

    static QWidget *parentWidget()
    {
        const QWidgetList widgets = QApplication::topLevelWidgets();

        if (widgets.count())
            return widgets.at(0);
        else
            return nullptr;
    }

    static bool question(const QString& title, const QString& text)
    {
        MessageBox msgBox(parentWidget());
        msgBox.setWindowTitle(title);
        msgBox.setText(text);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);

        return (msgBox.exec() == QMessageBox::Yes);
    }


    static void warning(const QString& title, const QString& text)
    {
        Q_UNUSED(title)
        Q_UNUSED(text)
        QMessageBox::warning(parentWidget(), tr("LXQt Power Manager Error"), tr("Hibernate failed."));
    }
};

PowerManager::PowerManager(QObject * parent, bool skipWarning)
    : QObject(parent),
        m_skipWarning(skipWarning)
{
    m_power = new Power(this);
//    connect(m_power, SIGNAL(suspendFail()), this, SLOT(suspendFailed()));
//    connect(m_power, SIGNAL(hibernateFail()), this, SLOT(hibernateFailed()));
//    connect(m_power, SIGNAL(monitoring(const QString &)),
//            this, SLOT(monitoring(const QString&)));

    QString sessionConfig(QFile::decodeName(qgetenv("LXQT_SESSION_CONFIG")));
    Settings settings(sessionConfig.isEmpty() ? QL1S("session") : sessionConfig);
    m_skipWarning = settings.value(QL1S("leave_confirmation")).toBool() ? false : true;
}

PowerManager::~PowerManager()
{
//    delete m_power;
}

QList<QAction*> PowerManager::availableActions()
{
    QList<QAction*> ret;
    QAction * act;

    // TODO/FIXME: icons
    if (m_power->canHibernate())
    {
        act = new QAction(XdgIcon::fromTheme(QL1S("system-suspend-hibernate")), tr("Hibernate"), this);
        connect(act, &QAction::triggered, this, &PowerManager::hibernate);
        ret.append(act);
    }

    if (m_power->canSuspend())
    {
        act = new QAction(XdgIcon::fromTheme(QL1S("system-suspend")), tr("Suspend"), this);
        connect(act, &QAction::triggered, this, &PowerManager::suspend);
        ret.append(act);
    }

    if (m_power->canReboot())
    {
        act = new QAction(XdgIcon::fromTheme(QL1S("system-reboot")), tr("Reboot"), this);
        connect(act, &QAction::triggered, this, &PowerManager::reboot);
        ret.append(act);
    }

    if (m_power->canShutdown())
    {
        act = new QAction(XdgIcon::fromTheme(QL1S("system-shutdown")), tr("Shutdown"), this);
        connect(act, &QAction::triggered, this, &PowerManager::shutdown);
        ret.append(act);
    }

    if (m_power->canLogout())
    {
        act = new QAction(XdgIcon::fromTheme(QL1S("system-log-out")), tr("Logout"), this);
        connect(act, &QAction::triggered, this, &PowerManager::logout);
        ret.append(act);
    }

    return ret;
}


void PowerManager::suspend()
{
     if (m_skipWarning ||
         MessageBox::question(tr("LXQt Session Suspend"),
                              tr("Do you want to really suspend your computer?<p>Suspends the computer into a low power state. System state is not preserved if the power is lost.")))
    {
        m_power->suspend();
    }
}

void PowerManager::hibernate()
{
    if (m_skipWarning ||
        MessageBox::question(tr("LXQt Session Hibernate"),
                             tr("Do you want to really hibernate your computer?<p>Hibernates the computer into a low power state. System state is preserved if the power is lost.")))
    {
        m_power->hibernate();
    }
}

void PowerManager::reboot()
{
    if (m_skipWarning ||
        MessageBox::question(tr("LXQt Session Reboot"),
                             tr("Do you want to really restart your computer? All unsaved work will be lost...")))
    {
        m_power->reboot();
    }
}

void PowerManager::shutdown()
{
    if (m_skipWarning ||
        MessageBox::question(tr("LXQt Session Shutdown"),
                             tr("Do you want to really switch off your computer? All unsaved work will be lost...")))
    {
        m_power->shutdown();
    }
}

void PowerManager::logout()
{
    if (m_skipWarning ||
        MessageBox::question(tr("LXQt Session Logout"),
                             tr("Do you want to really logout? All unsaved work will be lost...")))
    {
        m_power->logout();
    }
}

void PowerManager::hibernateFailed()
{
    MessageBox::warning(tr("LXQt Power Manager Error"), tr("Hibernate failed."));
}

void PowerManager::suspendFailed()
{
    MessageBox::warning(tr("LXQt Power Manager Error"), tr("Suspend failed."));
}

} // namespace LXQt
