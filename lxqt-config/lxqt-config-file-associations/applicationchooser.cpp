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

#include <QDialogButtonBox>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QDebug>
#include <QMimeDatabase>
#include <QTimer>

#include <XdgDesktopFile>
#include <XdgMimeApps>

#include <algorithm>
#include <QPushButton>
#include "applicationchooser.h"

Q_DECLARE_METATYPE(XdgDesktopFile*)
ApplicationChooser::ApplicationChooser(const QString& type, bool showUseAlwaysCheckBox)
{
    widget.setupUi(this);
    widget.buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Ok"));
    widget.buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Cancel"));
    m_Type = type;
    XdgMimeApps appsDb;
    m_CurrentDefaultApplication = appsDb.defaultApp(m_Type);
    QMimeDatabase db;
    XdgMimeType mimeInfo(db.mimeTypeForName(m_Type));
    if (mimeInfo.isValid())
    {
        widget.mimetypeIconLabel->setPixmap(mimeInfo.icon().pixmap(widget.mimetypeIconLabel->size()));
        widget.mimetypeLabel->setText(mimeInfo.comment());
    }
    else
    {
        widget.mimetypeIconLabel->setText(m_Type);
        widget.mimetypeLabel->hide();
    }

    widget.alwaysUseCheckBox->setVisible(showUseAlwaysCheckBox);
    widget.buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(false);
}

ApplicationChooser::~ApplicationChooser()
{
    qDeleteAll(allApps);
}

int ApplicationChooser::exec()
{
    show();
    fillApplicationListWidget();

    return QDialog::exec();
}



bool lessThan(XdgDesktopFile* a, XdgDesktopFile* b)
{
    return a && b && a->name().toLower() < b->name().toLower();
}

void ApplicationChooser::updateAllIcons() {
    // loading all icons is very time-consuming...
    QCoreApplication::processEvents();
    QTreeWidget* tree = widget.applicationTreeWidget;
    int updated = 0;
    int top_n = tree->topLevelItemCount();
    for(int top_i = 0; top_i < top_n; ++top_i) {
        QTreeWidgetItem* parent = tree->topLevelItem(top_i);
        int n = parent->childCount();
        for(int i = 0; i < n; ++i) {
            QTreeWidgetItem* item = parent->child(i);
            XdgDesktopFile* desktopFile = item->data(0, 32).value<XdgDesktopFile*>();
            if(Q_LIKELY(desktopFile != NULL && !desktopFile->icon().isNull())) {
                item->setIcon(0, desktopFile->icon());
                ++updated;
                if(updated % 8 == 0) // update the UI in batch is more efficient
                    QCoreApplication::processEvents();
            }
        }
    }
    QCoreApplication::processEvents();
    QApplication::restoreOverrideCursor();
}

void ApplicationChooser::fillApplicationListWidget()
{
    widget.applicationTreeWidget->clear();


    QSet<XdgDesktopFile*> addedApps;
    XdgMimeApps appsDb;
    QList<XdgDesktopFile*> applicationsThatHandleThisMimetype = appsDb.apps(m_Type);

    // Adding all apps takes some time. Make the user aware by setting the
    // cursor to Wait.
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QMimeDatabase db;
    XdgMimeType mimeInfo(db.mimeTypeForName(m_Type));
    if (mimeInfo.isValid())
    {
        QStringList mimetypes;
        mimetypes << m_Type << mimeInfo.allAncestors();

        for(const QString& mts : qAsConst(mimetypes)) {
            QMimeType mt = db.mimeTypeForName(mts);
            QString heading = mt.name() == QLatin1String("application/octet-stream") ?
                tr("Other applications") :
                tr("Applications that handle %1").arg(mt.comment());

            QList<XdgDesktopFile*> applications = mt.name() == QLatin1String("application/octet-stream") ?
                appsDb.allApps() :
                appsDb.recommendedApps(mt.name());

            std::sort(applications.begin(), applications.end(), lessThan);

            QTreeWidgetItem* headingItem = new QTreeWidgetItem(widget.applicationTreeWidget);
            headingItem->setExpanded(true);
            headingItem->setFlags(Qt::ItemIsEnabled);
            headingItem->setText(0, heading);
            headingItem->setSizeHint(0, QSize(0, 25));

            addApplicationsToApplicationListWidget(headingItem, applications, addedApps);
        }
    }
    else // schemes
    {
        QStringList types;
        types << m_Type << QString();
        for(const QString& type : qAsConst(types)) {
            QString heading = type.isEmpty() ?
                tr("Other applications") :
                tr("Applications that handle %1").arg(type);
            QList<XdgDesktopFile*> applications = type.isEmpty() ?
                appsDb.allApps() :
                appsDb.recommendedApps(type);

            std::sort(applications.begin(), applications.end(), lessThan);

            QTreeWidgetItem* headingItem = new QTreeWidgetItem(widget.applicationTreeWidget);
            headingItem->setExpanded(true);
            headingItem->setFlags(Qt::ItemIsEnabled);
            headingItem->setText(0, heading);
            headingItem->setSizeHint(0, QSize(0, 25));

            addApplicationsToApplicationListWidget(headingItem, applications, addedApps);
        }
    }
    connect(widget.applicationTreeWidget, &QTreeWidget::currentItemChanged,
            this, &ApplicationChooser::selectionChanged);
    widget.applicationTreeWidget->setFocus();

    if (!applicationsThatHandleThisMimetype.isEmpty()) {
        widget.buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(true);
        qDeleteAll(applicationsThatHandleThisMimetype);
        applicationsThatHandleThisMimetype.clear();
    }

    // delay icon update for faster loading
    QTimer::singleShot(0, this, &ApplicationChooser::updateAllIcons);
}

void ApplicationChooser::addApplicationsToApplicationListWidget(QTreeWidgetItem* parent,
                                                                QList<XdgDesktopFile*> applications,
                                                                QSet<XdgDesktopFile*>& alreadyAdded)
{
    QIcon placeHolderIcon = QIcon::fromTheme(QStringLiteral("application-x-executable"));
    bool noApplication = applications.isEmpty();
    bool inserted = false;

    // Insert applications in the listwidget, skipping already added applications
    // as well as desktop files that aren't applications
    while (!applications.isEmpty())
    {
        XdgDesktopFile* desktopFile = applications.first();

        // Only applications
        if (desktopFile->type() != XdgDesktopFile::ApplicationType)
        {
            delete applications.takeFirst();
            continue;
        }

        // WARNING: We cannot use QSet::contains() here because different addresses
        // can have the same value. Also, see libqtxdg -> XdgDesktopFile::operator==().
        bool wasAdded = false;
        for (XdgDesktopFile* added : qAsConst(alreadyAdded))
        {
            if (*added == *desktopFile)
            {
                wasAdded = true;
                break;
            }
        }
        if (wasAdded) {
            delete applications.takeFirst();
            continue;
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(parent);
        item->setIcon(0, placeHolderIcon);
        item->setText(0, desktopFile->name());
        item->setData(0, 32, QVariant::fromValue<XdgDesktopFile*>(desktopFile));

        if (widget.applicationTreeWidget->selectedItems().isEmpty()
            && m_CurrentDefaultApplication
            && *desktopFile == *m_CurrentDefaultApplication)
        {
            widget.applicationTreeWidget->setCurrentItem(item);
        }

        inserted = true;
        alreadyAdded.insert(desktopFile);

        allApps.insert(desktopFile);
        applications.removeFirst();
    }

    if (!inserted)
    {
        QTreeWidgetItem* noAppsFoundItem = new QTreeWidgetItem(parent);
        if (noApplication || alreadyAdded.isEmpty())
            noAppsFoundItem->setText(0, tr("No applications found"));
        else
        {
            // in this case, applications are found but were already added;
            // so, the text should be a little different
            noAppsFoundItem->setText(0, tr("No more applications found"));
        }
        noAppsFoundItem->setFlags(Qt::NoItemFlags);
        QFont font = noAppsFoundItem->font(0);
        font.setStyle(QFont::StyleItalic);
        noAppsFoundItem->setFont(0, font);
    }
}

void ApplicationChooser::selectionChanged()
{
    widget.buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(false);

    QTreeWidgetItem* newItem = widget.applicationTreeWidget->currentItem();
    if (newItem && newItem->data(0, 32).value<XdgDesktopFile*>())
    {
        widget.buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(true);

        // in d-tor, we delete all app pointers except for m_CurrentDefaultApplication
        // because it needs to be returned (and deleted by the caller)
        allApps.insert(m_CurrentDefaultApplication);
        m_CurrentDefaultApplication = newItem->data(0, 32).value<XdgDesktopFile*>();
        allApps.remove(m_CurrentDefaultApplication);
    }
}
