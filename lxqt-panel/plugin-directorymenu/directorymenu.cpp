/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2015 LXQt team
 * Authors:
 *   Daniel Drzisga <sersmicro@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is diinstributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <vector>

#include "directorymenu.h"
#include <QDebug>
#include <QDesktopServices>
#include <QProcess>
#include <QStringList>
#include <QFileInfo>
#include <QUrl>
#include <QIcon>

#include <XdgIcon>

DirectoryMenu::DirectoryMenu(const ILXQtPanelPluginStartupInfo &startupInfo) :
    QObject(),
    ILXQtPanelPlugin(startupInfo),
    mMenu(0),
    mDefaultIcon(XdgIcon::fromTheme(QStringLiteral("folder")))
{
    mOpenDirectorySignalMapper = new QSignalMapper(this);
    mOpenTerminalSignalMapper = new QSignalMapper(this);
    mMenuSignalMapper = new QSignalMapper(this);

    mButton.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mButton.setAutoRaise(true);
    mButton.setIcon(XdgIcon::fromTheme(QStringLiteral("folder")));

    connect(&mButton, SIGNAL(clicked()), this, SLOT(showMenu()));
    connect(mOpenDirectorySignalMapper, SIGNAL(mapped(QString)), this, SLOT(openDirectory(QString)));
    connect(mOpenTerminalSignalMapper, SIGNAL(mapped(QString)), this, SLOT(openInTerminal(QString)));
    connect(mMenuSignalMapper, SIGNAL(mapped(QString)), this, SLOT(addMenu(QString)));

    settingsChanged();
}

DirectoryMenu::~DirectoryMenu()
{
    delete mMenu;
}

void DirectoryMenu::showMenu()
{
    if(mBaseDirectory.exists())
    {
        buildMenu(mBaseDirectory.absolutePath());
    }
    else
    {
        buildMenu(QDir::homePath());
    }

    willShowWindow(mMenu);
    // Just using Qt`s activateWindow() won't work on some WMs like Kwin.
    // Solution is to execute menu 1ms later using timer
    mMenu->popup(calculatePopupWindowPos(mMenu->sizeHint()).topLeft());
}

void DirectoryMenu::buildMenu(const QString& path)
{
    delete mMenu;

    mPathStrings.clear();

    mMenu = new QMenu();

    addActions(mMenu, path);
}

void DirectoryMenu::openDirectory(const QString& path)
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("file://") + QDir::toNativeSeparators(path)));
}

void DirectoryMenu::openInTerminal(const QString& path)
{
    // Create list of arguments
    QStringList args;
    args << QStringLiteral("--workdir") << QDir::toNativeSeparators(path);
    // Execute the default terminal program with arguments
    QProcess::startDetached(mDefaultTerminal, args);
}

void DirectoryMenu::addMenu(QString path)
{
    QSignalMapper* sender = (QSignalMapper* )QObject::sender();
    QMenu* parentMenu = (QMenu*) sender->mapping(path);

    if(parentMenu->isEmpty())
    {
        addActions(parentMenu, path);
    }
}

void DirectoryMenu::addActions(QMenu* menu, const QString& path)
{
    mPathStrings.push_back(path);

    QAction* openDirectoryAction = menu->addAction(XdgIcon::fromTheme(QStringLiteral("folder")), tr("Open"));
    connect(openDirectoryAction, SIGNAL(triggered()), mOpenDirectorySignalMapper, SLOT(map()));
    mOpenDirectorySignalMapper->setMapping(openDirectoryAction, mPathStrings.back());

    QAction* openTerminalAction = menu->addAction(XdgIcon::fromTheme(QStringLiteral("folder")), tr("Open in terminal"));
    connect(openTerminalAction, SIGNAL(triggered()), mOpenTerminalSignalMapper, SLOT(map()));
    mOpenTerminalSignalMapper->setMapping(openTerminalAction, mPathStrings.back());

    menu->addSeparator();

    QDir dir(path);
    const QFileInfoList list = dir.entryInfoList();

    for (const QFileInfo& entry : list)
    {
        if(entry.isDir() && !entry.isHidden())
        {
            mPathStrings.push_back(entry.fileName());

            QMenu* subMenu = menu->addMenu(XdgIcon::fromTheme(QStringLiteral("folder")), mPathStrings.back());

            connect(subMenu, SIGNAL(aboutToShow()), mMenuSignalMapper, SLOT(map()));
            mMenuSignalMapper->setMapping(subMenu, entry.absoluteFilePath());
        }
    }
}

QDialog* DirectoryMenu::configureDialog()
{
     return new DirectoryMenuConfiguration(settings());
}

void DirectoryMenu::settingsChanged()
{
    mBaseDirectory.setPath(settings()->value(QStringLiteral("baseDirectory"), QDir::homePath()).toString());

    QString iconPath = settings()->value(QStringLiteral("icon"), QString()).toString();
    QIcon icon = QIcon(iconPath);

    if(!icon.isNull())
    {
        QIcon buttonIcon = QIcon(icon);
        if(!buttonIcon.pixmap(QSize(24,24)).isNull())
        {
            mButton.setIcon(buttonIcon);
            return;
        }
    }

    mButton.setIcon(mDefaultIcon);

    // Set default terminal
    mDefaultTerminal = settings()->value(QStringLiteral("defaultTerminal"), QString()).toString();
}
