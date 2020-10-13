/***************************************************************************
 *   Copyright (C) 2009 - 2013 by Artem 'DOOMer' Galichkin                 *
 *   doomer3d@gmail.com                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef EXTEDIT_H
#define EXTEDIT_H

#include <QObject>
#include <QProcess>
#include <QFileSystemWatcher>
#include <QAction>
#include <qt5xdg/XdgDesktopFile>
#include <qt5xdg/XdgAction>

class ExtEdit : public QObject
{
    Q_OBJECT
public:
    explicit ExtEdit(QObject *parent = 0);
    QList<XdgAction*> getActions();

public Q_SLOTS:
    void runExternalEditor();

private Q_SLOTS:
    void closedExternalEditor(int exitCode, QProcess::ExitStatus exitStatus);
    void editedFileChanged(const QString & path);

private:
    void createAppList();

    QList<XdgDesktopFile*> _appList;
    QList<XdgAction*> _actionList;
    QString _editFilename;
    bool _fileIsChanged;
    QFileSystemWatcher *_watcherEditedFile;
};

#endif // EXTEDIT_H
