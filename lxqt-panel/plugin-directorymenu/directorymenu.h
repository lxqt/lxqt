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


#ifndef DIRECTORYMENU_H
#define DIRECTORYMENU_H

#include "../panel/ilxqtpanelplugin.h"
 #include "directorymenuconfiguration.h"

#include <QLabel>
#include <QToolButton>
#include <QDomElement>
#include <QAction>
#include <QDir>
#include <QSignalMapper>
#include <QSettings>
#include <QMenu>

class DirectoryMenu :  public QObject, public ILXQtPanelPlugin
{
    Q_OBJECT

public:
    DirectoryMenu(const ILXQtPanelPluginStartupInfo &startupInfo);
    ~DirectoryMenu();

    virtual QWidget *widget() { return &mButton; }
    virtual QString themeId() const { return QStringLiteral("DirectoryMenu"); }
    virtual ILXQtPanelPlugin::Flags flags() const { return HaveConfigDialog; }
    QDialog *configureDialog();
    void settingsChanged();

private slots:
    void showMenu();
    void openDirectory(const QString& path);
    void openInTerminal(const QString &path);
    void addMenu(QString path);

protected slots:
    void buildMenu(const QString& path);

private:
	void addActions(QMenu* menu, const QString& path);

    QToolButton mButton;
    QMenu *mMenu;
    QSignalMapper *mOpenDirectorySignalMapper;
    QSignalMapper *mOpenTerminalSignalMapper; // New singal mapper to opening direcotry in term
    QSignalMapper *mMenuSignalMapper;

    QDir mBaseDirectory;
    QIcon mDefaultIcon;
    std::vector<QString> mPathStrings;
    QString mDefaultTerminal;
};

class DirectoryMenuLibrary: public QObject, public ILXQtPanelPluginLibrary
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "lxqt.org/Panel/PluginInterface/3.0")
    Q_INTERFACES(ILXQtPanelPluginLibrary)
public:
    ILXQtPanelPlugin *instance(const ILXQtPanelPluginStartupInfo &startupInfo) const
    {
        return new DirectoryMenu(startupInfo);
    }
};


#endif

