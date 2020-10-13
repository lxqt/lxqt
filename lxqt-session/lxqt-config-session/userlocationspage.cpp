/*
 * This file is part of the LXQt project. <https://lxqt.org>
 * Copyright (C) 2015 Luís Pereira <luis.artur.pereira@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "userlocationspage.h"

#include <XdgDirs>
#include <XdgIcon>

#include <QCoreApplication>
#include <QLabel>
#include <QSignalMapper>
#include <QLineEdit>
#include <QToolButton>
#include <QStringList>
#include <QGridLayout>
#include <QSpacerItem>
#include <QFileDialog>
#include <QMessageBox>


class UserLocationsPagePrivate {
public:

    UserLocationsPagePrivate();
    static char const * const locationsName[];
    static char const * const locationsToolTips[];

    QList<QString> initialLocations;
    QList<QLineEdit *> locations;
    QSignalMapper *signalMapper;

    void getUserDirs();
    void populate();
};

UserLocationsPagePrivate::UserLocationsPagePrivate()
    : locations(),
      signalMapper(nullptr)
{
}

// Note: strings can't actually be translated here (in static initialization time)
// the QT_TR_NOOP here is just for qt translate tools to get the strings for translation

// This labels haveto match XdgDirs::UserDirectories
char const * const UserLocationsPagePrivate::locationsName[] = {
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Desktop"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Downloads"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Templates"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Public Share"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Documents"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Music"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Pictures"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Videos")};

char const * const UserLocationsPagePrivate::locationsToolTips[] = {
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Contains all the files which you see on your desktop"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Default folder to save your downloaded files"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Default folder to load or save templates from or to"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Default folder to publicly share your files"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Default folder to load or save documents from or to"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Default folder to load or save music from or to"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Default folder to load or save pictures from or to"),
    QT_TRANSLATE_NOOP("UserLocationsPrivate", "Default folder to load or save videos from or to")};

static constexpr int locationsNameCount = sizeof (UserLocationsPagePrivate::locationsName) / sizeof (UserLocationsPagePrivate::locationsName[0]);
static_assert (locationsNameCount == sizeof (UserLocationsPagePrivate::locationsToolTips) / sizeof (UserLocationsPagePrivate::locationsToolTips[0])
            , "Size of UserLocationsPagePrivate::locationsName & UserLocationsPagePrivate::locationsToolTips must match");

void UserLocationsPagePrivate::getUserDirs()
{
    for(int i = 0; i < locationsNameCount; ++i) {
        const QString userDir = XdgDirs::userDir(static_cast<XdgDirs::UserDirectory> (i));
        const QDir dir(userDir);
        initialLocations.append(dir.canonicalPath());
    }
}

void UserLocationsPagePrivate::populate()
{
    const int N = initialLocations.count();

    Q_ASSERT(N == locationsNameCount);

    for (int i = 0; i < N; ++i) {
        locations.at(i)->setText(initialLocations.at(i));
    }
}


UserLocationsPage::UserLocationsPage(QWidget *parent)
    : QWidget(parent),
      d(new UserLocationsPagePrivate())
{
    d->signalMapper = new QSignalMapper(this);
    QGridLayout *gridLayout = new QGridLayout(this);

    int row = 0;

    QLabel *description = new QLabel(tr("Locations for Personal Files"), this);
    QFont font;
    font.setBold(true);
    description->setFont(font);

    gridLayout->addWidget(description, row++, 0, 1, -1);

    for (int i = 0; i < locationsNameCount; ++i, ++row) {
        QLabel *label = new QLabel(QCoreApplication::translate("UserLocationsPrivate", d->locationsName[i]), this);

        QLineEdit *edit = new QLineEdit(this);
        d->locations.append(edit);
        edit->setClearButtonEnabled(true);
        edit->setToolTip(QCoreApplication::translate("UserLocationsPrivate", d->locationsToolTips[i]));

        QToolButton *button = new QToolButton(this);
        button->setIcon(XdgIcon::fromTheme(QStringLiteral("folder")));
        connect(button, SIGNAL(clicked()), d->signalMapper, SLOT(map()));
        d->signalMapper->setMapping(button, i);

        gridLayout->addWidget(label, row, 0);
        gridLayout->addWidget(edit, row, 1);
        gridLayout->addWidget(button, row, 2);
    }
    connect(d->signalMapper, SIGNAL(mapped(int)),
            this, SLOT(clicked(int)));

    QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum,
                                                  QSizePolicy::Expanding);
    gridLayout->addItem(verticalSpacer, row++, 1, 1, 1);
    setLayout(gridLayout);

    d->getUserDirs();
    d->populate();
}

UserLocationsPage::~UserLocationsPage()
{
    // It's fine to delete a null pointer. No need to check.
    delete d;
    d = nullptr;
}

void UserLocationsPage::restoreSettings()
{
    d->populate();
}

void UserLocationsPage::save()
{
    bool restartWarn = false;

    const int N = d->locations.count();
    for (int i = 0; i < N; ++i) {
        QString s;
        const QString text = d->locations.at(i)->text();

        if (text.isEmpty()) {
            s = XdgDirs::userDirDefault(static_cast<XdgDirs::UserDirectory> (i));
        } else {
            const QDir dir(text);
            s = dir.canonicalPath();
        }

        if (s != d->initialLocations.at(i)) {
            const bool ok = XdgDirs::setUserDir(
                        static_cast<XdgDirs::UserDirectory> (i), s, true);
            if (!ok) {
                const int ret = QMessageBox::warning(this,
                    tr("LXQt Session Settings - User Directories"),
                    tr("An error ocurred while applying the settings for the %1 location").arg(QCoreApplication::translate("UserLocationsPrivate", d->locationsName[i])),
                    QMessageBox::Ok);
                Q_UNUSED(ret);
            }
            restartWarn = true;
        }
    }

    if (restartWarn)
        emit needRestart();
}

void UserLocationsPage::clicked(int id)
{
    const QString& currentDir = d->locations.at(id)->text();
    const QString dir = QFileDialog::getExistingDirectory(this,
                    tr("Choose Location"),
                    currentDir,
                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        const QDir dd(dir);
        d->locations.at(id)->setText(dd.canonicalPath());
    }
}
