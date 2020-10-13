/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Christian Surlykke
 *            2014 Luís Pereira <luis.artur.pereira.gmail.com>
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

#ifndef _MIMETYPEVIEWER_H
#define	_MIMETYPEVIEWER_H

#include <QDialog>
#include <QStringList>
#include <QTemporaryFile>

#include <XdgMimeType>

#include "mimetypedata.h"
#include "ui_mimetypeviewer.h"

class QSettings;

namespace LXQt {
class SettingsCache;
}

class MimetypeViewer : public QDialog {
    Q_OBJECT
public:
    MimetypeViewer(QWidget *parent = 0);
    virtual ~MimetypeViewer();

private slots:
    void currentMimetypeChanged();
    void filter(const QString&);
    void chooseApplication();
    void dialogButtonBoxClicked(QAbstractButton *button);

private:
    XdgDesktopFile* chooseApp(const QString& type);
    void initializeMimetypeTreeView();
    void updateDefaultApplications();
    void addSearchIcon();
    void loadAllMimeTypes();
    QString m_CurrentType;
    Ui::mimetypeviewer widget;
    QStringList mediaTypes;
    QTemporaryFile mMimeappsTemp, mDEMimeappsTemp; // for resetting all settings
    QList <QTreeWidgetItem*> mItemList;
    QMap<QString, QTreeWidgetItem *> mGroupItems;
};

#endif	/* _MIMETYPEVIEWER_H */
