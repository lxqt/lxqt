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

#include <QListWidgetItem>
#include <QDebug>
#include <QIcon>
#include <QPixmap>
#include <QListWidget>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDateTime>
#include <QFileInfo>
#include <QPushButton>
#include <XdgDesktopFile>
#include <XdgMimeApps>
#include <XdgDirs>

#include <algorithm>

#include "mimetypeviewer.h"
#include "ui_mimetypeviewer.h"

#include "applicationchooser.h"


enum ItemTypeEntries {
    GroupType = 1001,
    EntrieType = 1002
};

static bool mimeTypeLessThan(const QMimeType& m1, const QMimeType& m2)
{
    return m1.name() < m2.name();
}

void MimetypeViewer::loadAllMimeTypes()
{
    mediaTypes.clear();
    mGroupItems.clear();
    mItemList.clear();
    QStringList selectedMimeTypes;

    QMimeDatabase db;
    QList<QMimeType> mimetypes = db.allMimeTypes();

    std::sort(mimetypes.begin(), mimetypes.end(), mimeTypeLessThan);
    for (const QMimeType &mt : qAsConst(mimetypes)) {
        const QString mimetype = mt.name();
        const int i = mimetype.indexOf(QLatin1Char('/'));
        const QString mediaType = mimetype.left(i);
        const QString subType = mimetype.mid(i + 1);

        if (!mediaTypes.contains(mediaType)) { // A new type of media
            mediaTypes.append(mediaType);
            QTreeWidgetItem *item = new QTreeWidgetItem(widget.mimetypeTreeWidget, GroupType);
            item->setText(0, mediaType);
            widget.mimetypeTreeWidget->insertTopLevelItem(0, item);
            mGroupItems.insert(mediaType, item);
        }
        QTreeWidgetItem *item = new QTreeWidgetItem(mGroupItems.value(mediaType), EntrieType);
        QVariant v;
        v.setValue(MimeTypeData(mt));
        item->setData(0, Qt::UserRole, v);
        item->setText(0, subType);
        mItemList.append(item);
    }

    // Also, add some x-scheme-handler protocols (sorted alphabetically)
    static const QStringList schemes = {
                                        QStringLiteral("appstream"), QStringLiteral("apt"),
                                        QStringLiteral("ftp"), QStringLiteral("http"),
                                        QStringLiteral("https"), QStringLiteral("irc"),
                                        QStringLiteral("ircs"), QStringLiteral("magnet"),
                                        QStringLiteral("mailto"), QStringLiteral("mms"),
                                        QStringLiteral("mmsh"), QStringLiteral("mtp"),
                                        QStringLiteral("sip"), QStringLiteral("skype"),
                                        QStringLiteral("sftp"), QStringLiteral("smb"),
                                        QStringLiteral("ssh"), QStringLiteral("tg")
                                       };
    const QString mediaType = QStringLiteral("x-scheme-handler");
    if (!mediaTypes.contains(mediaType)) {
        mediaTypes.append(mediaType);
        QTreeWidgetItem *item = new QTreeWidgetItem(widget.mimetypeTreeWidget, GroupType);
        item->setText(0, mediaType);
        widget.mimetypeTreeWidget->insertTopLevelItem(0, item);
        mGroupItems.insert(mediaType, item);
    }
    for (const auto& scheme : schemes) {
        QTreeWidgetItem *item = new QTreeWidgetItem(mGroupItems.value(mediaType), EntrieType);
        MimeTypeData data = MimeTypeData();
        data.setName(mediaType + QStringLiteral("/") + scheme);
        QVariant v;
        v.setValue(data);
        item->setData(0, Qt::UserRole, v);
        item->setText(0, scheme);
        mItemList.append(item);
    }

    widget.mimetypeTreeWidget->resizeColumnToContents(1);
    widget.mimetypeTreeWidget->show();
}


MimetypeViewer::MimetypeViewer(QWidget *parent)
    : QDialog(parent)
{
    widget.setupUi(this);
    widget.buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    widget.buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    addSearchIcon();
    widget.searchTermLineEdit->setEnabled(false);
    connect(widget.searchTermLineEdit, &QLineEdit::textEdited, this, &MimetypeViewer::filter);
    connect(widget.chooseApplicationsButton, &QAbstractButton::clicked, this, &MimetypeViewer::chooseApplication);
    connect(widget.buttonBox_1, &QDialogButtonBox::clicked, this, &MimetypeViewer::dialogButtonBoxClicked);

    // remember the global apps list
    QString mimeappsListPath(XdgDirs::configHome(true) + QStringLiteral("/mimeapps.list"));
    QByteArray ba;
    QFile mimeFile(mimeappsListPath);
    if (mimeFile.open(QFile::ReadOnly))
    {
        ba = mimeFile.readAll();
        mimeFile.close();
    }
    mMimeappsTemp.open();
    QTextStream txtStream(&mMimeappsTemp);
    txtStream << ba;
    mMimeappsTemp.close();
    // remember the DE's default apps list
    QList<QByteArray> desktopsList = qgetenv("XDG_CURRENT_DESKTOP").toLower().split(':');
    if (!desktopsList.isEmpty())
    {
        ba.clear();
        QString DEMimeappsListPath(XdgDirs::configHome(true)
                                    + QStringLiteral("/")
                                    + QString::fromLocal8Bit(desktopsList.at(0))
                                    + QStringLiteral("-mimeapps.list"));
        QFile DEMimeFile(DEMimeappsListPath);
        if (DEMimeFile.open(QFile::ReadOnly))
        {
            ba = DEMimeFile.readAll();
            DEMimeFile.close();
        }
        mDEMimeappsTemp.open();
        QTextStream DETxtStream(&mDEMimeappsTemp);
        DETxtStream << ba;
        mDEMimeappsTemp.close();
    }

    initializeMimetypeTreeView();
    loadAllMimeTypes();
    widget.searchTermLineEdit->setFocus();

    connect(widget.mimetypeTreeWidget, &QTreeWidget::itemSelectionChanged, this, &MimetypeViewer::currentMimetypeChanged);

    // "Default Applications" tab
    updateDefaultApplications();
    connect(widget.chooseBrowserButton, &QAbstractButton::clicked, [this]() {
        if (XdgDesktopFile *app = chooseApp(QStringLiteral("x-scheme-handler/http")))
        { // also asign https and "text/html"
            XdgMimeApps appsDb;
            XdgDesktopFile *defaultApp = appsDb.defaultApp(QStringLiteral("x-scheme-handler/https"));
            bool typeChanged = false;
            if (!defaultApp || !defaultApp->isValid() || *defaultApp != *app)
            {
                appsDb.setDefaultApp(QStringLiteral("x-scheme-handler/https"), *app);
                typeChanged = true;
            }
            delete defaultApp;
            defaultApp = appsDb.defaultApp(QStringLiteral("text/html"));
            if (!defaultApp || !defaultApp->isValid() || *defaultApp != *app)
            {
                appsDb.setDefaultApp(QStringLiteral("text/html"), *app);
                typeChanged = true;
            }
            delete defaultApp;
            if (typeChanged)
                currentMimetypeChanged();
            delete app;
        }
    });
    connect(widget.chooseEmailClientButton, &QAbstractButton::clicked, [this]() {
        if (XdgDesktopFile *app = chooseApp(QStringLiteral("x-scheme-handler/mailto")))
            delete app;
    });
}

MimetypeViewer::~MimetypeViewer()
{
}

void MimetypeViewer::addSearchIcon()
{
    QIcon searchIcon = QIcon::fromTheme(QStringLiteral("system-search"));
    if (searchIcon.isNull())
        return;

    widget.searchTermLineEdit->setTextMargins(0, 0, 30, 0);
    QHBoxLayout *hBoxLayout = new QHBoxLayout(widget.searchTermLineEdit);
    hBoxLayout->setContentsMargins(0,0,0,0);
    widget.searchTermLineEdit->setLayout(hBoxLayout);
    QLabel *searchIconLabel = new QLabel(widget.searchTermLineEdit);
    searchIconLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    searchIconLabel->setMinimumHeight(30);
    searchIconLabel->setMinimumWidth(30);

    searchIconLabel->setPixmap(searchIcon.pixmap(QSize(20,20)));
    hBoxLayout->addWidget(searchIconLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
}


void MimetypeViewer::initializeMimetypeTreeView()
{
    currentMimetypeChanged();
    widget.mimetypeTreeWidget->setColumnCount(2);
    widget.searchTermLineEdit->setEnabled(true);
}

void MimetypeViewer::updateDefaultApplications()
{
    widget.browserIcon->clear();
    widget.browserIcon->hide();
    widget.emailClientIcon->clear();
    widget.emailClientIcon->hide();

    XdgMimeApps appsDb;
    bool defaultBrowserExists = false;
    XdgDesktopFile *defaultApp = nullptr;

    // for the default browser, check http and https scheme handlers as well as "text/html"
    defaultApp = appsDb.defaultApp(QStringLiteral("x-scheme-handler/http"));
    if (defaultApp && defaultApp->isValid())
    {
        XdgDesktopFile *df = appsDb.defaultApp(QStringLiteral("x-scheme-handler/https"));
        if (df && df->isValid() && *defaultApp == *df)
        {
            delete df;
            df = appsDb.defaultApp(QStringLiteral("text/html"));
            if (df && df->isValid() && *defaultApp == *df)
                defaultBrowserExists = true;
        }
        delete df;
    }
    if (defaultBrowserExists)
    {
        QString nonLocalizedName = defaultApp->value(QStringLiteral("Name")).toString();
        QString localizedName = defaultApp->localizedValue(QStringLiteral("Name"), nonLocalizedName).toString();
        QIcon appIcon = defaultApp->icon();
        widget.browserIcon->setPixmap(appIcon.pixmap(widget.browserIcon->size()));
        widget.browserIcon->show();
        widget.browserLabel->setText(localizedName);
        widget.chooseBrowserButton->setText(tr("&Change..."));
    }
    else
    {
        widget.browserLabel->setText(tr("None"));
        widget.chooseBrowserButton->setText(tr("&Choose..."));
    }
    delete defaultApp;

    // the default email client
    defaultApp = appsDb.defaultApp(QStringLiteral("x-scheme-handler/mailto"));
    if (defaultApp && defaultApp->isValid())
    {
        QString nonLocalizedName = defaultApp->value(QStringLiteral("Name")).toString();
        QString localizedName = defaultApp->localizedValue(QStringLiteral("Name"), nonLocalizedName).toString();
        QIcon appIcon = defaultApp->icon();
        widget.emailClientIcon->setPixmap(appIcon.pixmap(widget.emailClientIcon->size()));
        widget.emailClientIcon->show();
        widget.emailClientLabel->setText(localizedName);
        widget.chooseEmailClientButton->setText(tr("&Change..."));
    }
    else
    {
        widget.emailClientLabel->setText(tr("None"));
        widget.chooseEmailClientButton->setText(tr("&Choose..."));
    }
    delete defaultApp;
}

void MimetypeViewer::currentMimetypeChanged()
{
    // update the default apps tab if this function
    // is not called by changing the tree widget selection
    if (!qobject_cast<QTreeWidget*>(QObject::sender()))
        updateDefaultApplications();

    widget.iconLabel->hide();
    widget.descriptionLabel->setText(tr("None"));
    widget.mimetypeGroupBox->setEnabled(false);

    widget.patternsLabel->clear();
    widget.patternsGroupBox->setEnabled(false);

    widget.appIcon->hide();
    widget.applicationLabel->clear();
    widget.applicationsGroupBox->setEnabled(false);

    QTreeWidgetItem *sel = widget.mimetypeTreeWidget->currentItem();

    if (!sel || sel->type() == GroupType) {
        return;
    }

    MimeTypeData mimeData = sel->data(0, Qt::UserRole).value<MimeTypeData>();
    XdgMimeApps appsDb;
    XdgDesktopFile *defaultApp = nullptr;

    if (mimeData.name().startsWith(QStringLiteral("x-scheme-handler/")))
    {
        m_CurrentType = mimeData.name();
        defaultApp = appsDb.defaultApp(mimeData.name());
    }
    else
    {
        QMimeDatabase db;
        XdgMimeType mt = db.mimeTypeForName(mimeData.name());
        if (!mt.isValid())
            return;

        m_CurrentType = mimeData.name();
        defaultApp = appsDb.defaultApp(mimeData.name());

        widget.descriptionLabel->setText(mimeData.comment());

        QIcon icon = mt.icon();
        if (!icon.isNull())
        {
            widget.iconLabel->setPixmap(icon.pixmap(widget.iconLabel->size()));
            widget.iconLabel->show();
        }

        widget.mimetypeGroupBox->setEnabled(true);
        widget.patternsLabel->setText(mimeData.patterns());
        widget.patternsGroupBox->setEnabled(true);
    }

    if (defaultApp && defaultApp->isValid())
    {
        QString nonLocalizedName = defaultApp->value(QStringLiteral("Name")).toString();
        QString localizedName = defaultApp->localizedValue(QStringLiteral("Name"), nonLocalizedName).toString();
        QIcon appIcon = defaultApp->icon();
        widget.appIcon->setPixmap(appIcon.pixmap(widget.appIcon->size()));
        widget.appIcon->show();
        widget.applicationLabel->setText(localizedName);
        widget.chooseApplicationsButton->setText(tr("&Change..."));
    }
    else
    {
        widget.applicationLabel->setText(tr("None"));
        widget.chooseApplicationsButton->setText(tr("&Choose..."));
    }
    delete defaultApp;

    widget.applicationsGroupBox->setEnabled(true);
}

void MimetypeViewer::filter(const QString& pattern)
{
    QMimeDatabase db;
    MimeTypeData mimeData;

    for (int i = 0; i < widget.mimetypeTreeWidget->topLevelItemCount(); ++i) {
        widget.mimetypeTreeWidget->topLevelItem(i)->setHidden(true);
    }

    for(QTreeWidgetItem* it : qAsConst(mItemList)) {
        mimeData = it->data(0, Qt::UserRole).value<MimeTypeData>();
        if (pattern.isEmpty() || mimeData.matches(pattern)) {
            const int i = mimeData.name().indexOf(QLatin1Char('/'));
            const QString mediaType = mimeData.name().left(i);
            QTreeWidgetItem* groupItem = mGroupItems.value(mediaType);
            Q_ASSERT(groupItem);
            if (groupItem) {
                groupItem->setHidden(false);
                it->setHidden(false);
            }
        } else {
            it->setHidden(true);
        }
    }
}

void MimetypeViewer::chooseApplication()
{
    if (XdgDesktopFile *app = chooseApp(m_CurrentType))
        delete app;
}

XdgDesktopFile* MimetypeViewer::chooseApp(const QString& type)
{
    XdgDesktopFile *app = nullptr;
    ApplicationChooser applicationChooser(type);
    int dialogCode = applicationChooser.exec();
    app = applicationChooser.DefaultApplication();
    if (app)
    {
        if (dialogCode == QDialog::Accepted)
        {
            XdgMimeApps appsDb;
            XdgDesktopFile *defaultApp = appsDb.defaultApp(type);
            if (!defaultApp || !defaultApp->isValid() || *defaultApp != *app)
            {
                appsDb.setDefaultApp(type, *app);
                currentMimetypeChanged();
            }
            else
            {
                delete app; // no memory leak
                app = nullptr;
            }
            delete defaultApp; // no memory leak
        }
        else
        {
            delete app; // no memory leak
            app = nullptr;
        }
    }
    widget.mimetypeTreeWidget->setFocus();
    return app; // should be deleted by the caller
}

void MimetypeViewer::dialogButtonBoxClicked(QAbstractButton* button)
{
    QDialogButtonBox::ButtonRole role = widget.buttonBox_1->buttonRole(button);
    if (role == QDialogButtonBox::ResetRole)
    {
        // restore the global apps list
        QString mimeappsListPath(XdgDirs::configHome(true) + QStringLiteral("/mimeapps.list"));
        if (QFile::exists(mimeappsListPath))
            QFile::remove(mimeappsListPath);
        QFile::copy(mMimeappsTemp.fileName(), mimeappsListPath);
        // restore the DE's default apps list
        QList<QByteArray> desktopsList = qgetenv("XDG_CURRENT_DESKTOP").toLower().split(':');
        if (!desktopsList.isEmpty())
        {
            QString DEMimeappsListPath(XdgDirs::configHome(true)
                                        + QStringLiteral("/")
                                        + QString::fromLocal8Bit(desktopsList.at(0))
                                        + QStringLiteral("-mimeapps.list"));
            if (QFile::exists(DEMimeappsListPath))
                QFile::remove(DEMimeappsListPath);
            QFile::copy(mDEMimeappsTemp.fileName(), DEMimeappsListPath);
        }

        currentMimetypeChanged();
    }
    else
    {
        close();
    }
}

