/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2013 Christian Surlykke
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

#ifndef _APPLICATIONCHOOSER_H
#define	_APPLICATIONCHOOSER_H

#include "ui_applicationchooser.h"
#include <XdgDesktopFile>
#include <XdgMimeType>

class QSettings;

class ApplicationChooser : public QDialog
{
    Q_OBJECT
public:
    ApplicationChooser(const QString& type, bool showUseAlwaysCheckBox = false);

    virtual ~ApplicationChooser();

    XdgDesktopFile* DefaultApplication() const {
        return m_CurrentDefaultApplication; // should be deleted by the caller
    }

    virtual int exec();

private slots:
    void selectionChanged();
    void updateAllIcons();

private:
    void fillApplicationListWidget();

    void addApplicationsToApplicationListWidget(QTreeWidgetItem* parent,
                                                QList<XdgDesktopFile*> applications,
                                                QSet<XdgDesktopFile*> & alreadyAdded);
    QString m_Type;
    Ui::ApplicationChooser widget;
    XdgDesktopFile* m_CurrentDefaultApplication;
    QSet<XdgDesktopFile*> allApps; // all app pointers that should be deleted in d-tor
};

#endif	/* _APPLICATIONCHOOSER_H */
