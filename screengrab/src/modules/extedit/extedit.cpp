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

#include "extedit.h"
#include "core/core.h"

#include <XdgMimeApps>

#include <QDebug>
#include <QMimeDatabase>

ExtEdit::ExtEdit(QObject *parent) :
    QObject(parent), _watcherEditedFile(new QFileSystemWatcher(this))
{
    createAppList();
    _fileIsChanged = false;
    connect(_watcherEditedFile, &QFileSystemWatcher::fileChanged, this, &ExtEdit::editedFileChanged);
}

QList<XdgAction*> ExtEdit::getActions()
{
    return _actionList;
}

void ExtEdit::runExternalEditor()
{
    XdgAction *action = static_cast<XdgAction*>(sender());

    Core *core = Core::instance();
    QString format = core->config()->getSaveFormat();
    if (format.isEmpty())
        format = QLatin1String("png");

    _editFilename = core->getTempFilename(format);
    core->writeScreen(_editFilename, format, true);

    QProcess *execProcess = new QProcess(this);
    void (QProcess:: *signal)(int, QProcess::ExitStatus) = &QProcess::finished;
    connect(execProcess, signal, this, &ExtEdit::closedExternalEditor);

     execProcess->start(action->desktopFile().expandExecString().first(),
                        QStringList() << _editFilename);
    _watcherEditedFile->addPath(_editFilename);
}

void ExtEdit::closedExternalEditor(int, QProcess::ExitStatus)
{
    Core *core = Core::instance();

    if (_fileIsChanged == true)
        core->updatePixmap();

    _fileIsChanged = false;
    _watcherEditedFile->removePath(_editFilename);

    sender()->deleteLater();
    core->killTempFile();
    _editFilename.clear();
}

void ExtEdit::editedFileChanged(const QString&)
{
    _fileIsChanged = true;
}

void ExtEdit::createAppList()
{
    Core *core = Core::instance();
    QString format = core->config()->getSaveFormat();
    if (format.isEmpty())
        format = QLatin1String("png");

    QString fileName = _editFilename.isEmpty() ? core->getTempFilename(format) : _editFilename;
    QMimeDatabase db;
    XdgMimeApps mimeAppsDb;
    QMimeType mt = db.mimeTypeForFile(fileName);
    _appList = mimeAppsDb.apps(mt.name());

    for (XdgDesktopFile *app : qAsConst(_appList))
        _actionList << new XdgAction(app);
}
