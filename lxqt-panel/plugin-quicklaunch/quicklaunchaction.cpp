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

#include "quicklaunchaction.h"
#include <QDesktopServices>
#include <QFileIconProvider>
#include <QMimeDatabase>
#include <QMessageBox>
#include <QProcess>
#include <QUrl>
#include <XdgDesktopFile>
#include <XdgIcon>
#include <XdgMimeType>


QuickLaunchAction::QuickLaunchAction(const QString & name,
                                     const QString & exec,
                                     const QString & icon,
                                     QWidget * parent)
    : QAction(name, parent),
      m_valid(true)
{
    m_type = ActionLegacy;

    m_settingsMap[QStringLiteral("name")] = name;
    m_settingsMap[QStringLiteral("exec")] = exec;
    m_settingsMap[QStringLiteral("icon")] = icon;

    if (icon == QLatin1String("") || icon.isNull())
        setIcon(XdgIcon::defaultApplicationIcon());
    else
        setIcon(QIcon(icon));

    setData(exec);
    connect(this, &QAction::triggered, this, [this] { execAction(); });
}

QuickLaunchAction::QuickLaunchAction(const XdgDesktopFile * xdg,
                                     QWidget * parent)
    : QAction(parent),
      m_valid(true)
{
    m_type = ActionXdg;

    m_settingsMap[QStringLiteral("desktop")] = xdg->fileName();

    QString title(xdg->localizedValue(QStringLiteral("Name")).toString());
    QString gn(xdg->localizedValue(QStringLiteral("GenericName")).toString());
    if (!gn.isEmpty())
        title += QLatin1String(" (") + gn + QLatin1String(")");
    setText(title);

    setIcon(xdg->icon(XdgIcon::defaultApplicationIcon()));

    setData(xdg->fileName());
    connect(this, &QAction::triggered, this, [this] { execAction(); });

    // populate the additional actions
    for (auto const & action : const_cast<const QStringList &&>(xdg->actions()))
    {
        QAction * act = new QAction{xdg->actionIcon(action), xdg->actionName(action), this};
        act->setData(action);
        connect(act, &QAction::triggered, [this, act] { execAction(act->data().toString()); });
        m_addtitionalActions.push_back(act);
    }
}

QuickLaunchAction::QuickLaunchAction(const QString & fileName, QWidget * parent)
    : QAction(parent),
      m_valid(true)
{
    m_type = ActionFile;
    setText(fileName);
    setData(fileName);

    m_settingsMap[QStringLiteral("file")] = fileName;

    QFileInfo fi(fileName);
    if (fi.isDir())
    {
        QFileIconProvider ip;
        setIcon(ip.icon(fi));
    }
    else
    {
        QMimeDatabase db;
        XdgMimeType mi(db.mimeTypeForFile(fi));
        setIcon(mi.icon());
    }

    connect(this, &QAction::triggered, this, [this] { execAction(); });
}

void QuickLaunchAction::execAction(QString additionalAction)
{
    QString exec(data().toString());
    qDebug() << "execAction" << exec;
    switch (m_type)
    {
        case ActionLegacy:
            QProcess::startDetached(exec);
            break;
        case ActionXdg:
        {
            XdgDesktopFile xdg;
            if(xdg.load(exec))
            {
                if (additionalAction.isEmpty())
                    xdg.startDetached();
                else
                    xdg.actionActivate(additionalAction, QStringList{});
            }
            break;
        }
        case ActionFile:
            QDesktopServices::openUrl(QUrl(exec));
            break;
    }
}
