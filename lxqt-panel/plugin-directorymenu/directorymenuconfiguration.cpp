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


#include <QIcon>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <XdgIcon>

#include "directorymenuconfiguration.h"
#include "ui_directorymenuconfiguration.h"


DirectoryMenuConfiguration::DirectoryMenuConfiguration(PluginSettings *settings, QWidget *parent) :
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::DirectoryMenuConfiguration),
    mBaseDirectory(QDir::homePath()),
    mDefaultIcon(XdgIcon::fromTheme(QStringLiteral("folder"))),
    mDefaultTerminal(QStringLiteral("/usr/bin/qterminal"))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("DirectoryMenuConfigurationWindow"));
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    connect(ui->buttonBox_1, SIGNAL(clicked(QAbstractButton*)), SLOT(dialogButtonsAction(QAbstractButton*)));
    loadSettings();
    ui->baseDirectoryB->setIcon(mDefaultIcon);

    connect(ui->baseDirectoryB, SIGNAL(clicked()), SLOT(showDirectoryDialog()));
    connect(ui->iconB, SIGNAL(clicked()), SLOT(showIconDialog()));
    connect(ui->terminalB, SIGNAL(clicked()), SLOT(showTermDialog()));
}

DirectoryMenuConfiguration::~DirectoryMenuConfiguration()
{
    delete ui;
}

void DirectoryMenuConfiguration::loadSettings()
{
    mBaseDirectory.setPath(settings().value(QStringLiteral("baseDirectory"), QDir::homePath()).toString());
    ui->baseDirectoryB->setText(mBaseDirectory.dirName());

    mIcon = settings().value(QStringLiteral("icon"), QString()).toString();
    if(!mIcon.isNull())
    {
        QIcon buttonIcon = QIcon(mIcon);
        if(!buttonIcon.pixmap(QSize(24,24)).isNull())
        {
            ui->iconB->setIcon(buttonIcon);
            return;
        }
    }

    ui->iconB->setIcon(mDefaultIcon);

    ui->terminalB->setText(settings().value(QStringLiteral("defaultTerminal"), QString()).toString());
}

void DirectoryMenuConfiguration::saveSettings()
{
    settings().setValue(QStringLiteral("baseDirectory"), mBaseDirectory.absolutePath());
    settings().setValue(QStringLiteral("icon"), mIcon);
    settings().setValue(QStringLiteral("defaultTerminal"), mDefaultTerminal);
}

void DirectoryMenuConfiguration::showDirectoryDialog()
{
    QFileDialog d(this, tr("Choose Base Directory"), mBaseDirectory.absolutePath());
    d.setFileMode(QFileDialog::Directory);
    d.setOptions(QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    d.setWindowModality(Qt::WindowModal);

    if(d.exec() && !d.selectedFiles().isEmpty())
    {
        mBaseDirectory.setPath(d.selectedFiles().constFirst());
        ui->baseDirectoryB->setText(mBaseDirectory.dirName());

        saveSettings();
    }
}

void DirectoryMenuConfiguration::showTermDialog()
{
    QFileDialog d(this, tr("Choose Default Terminal"), QStringLiteral("/usr/bin"));
    d.setFileMode(QFileDialog::ExistingFile);
    d.setWindowModality(Qt::WindowModal);
    
    if (d.exec() && !d.selectedFiles().isEmpty())
    {
        mDefaultTerminal = d.selectedFiles().constFirst();
        saveSettings();
    }
    ui->terminalB->setText(mDefaultTerminal);
}

void DirectoryMenuConfiguration::showIconDialog()
{
    QFileDialog d(this, tr("Choose Icon"), QDir::homePath(), tr("Icons (*.png *.xpm *.jpg)"));
    d.setWindowModality(Qt::WindowModal);

    if(d.exec() && !d.selectedFiles().isEmpty())
    {
        QIcon newIcon = QIcon(d.selectedFiles().constFirst());

        if(newIcon.pixmap(QSize(24,24)).isNull())
        {
            QMessageBox::warning(this, tr("Directory Menu"), tr("An error occurred while loading the icon."));
            return;
        }

        ui->iconB->setIcon(newIcon);
        mIcon = d.selectedFiles().constFirst();
        saveSettings();
    }
}
