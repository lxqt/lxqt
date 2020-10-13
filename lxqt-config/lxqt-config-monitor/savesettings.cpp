/*
    Copyright (C) 2015  P.L. Lucas <selairi@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <QJsonArray>
#include <QJsonObject>
#include "savesettings.h"
#include "configure.h"
#include <QDebug>
#include <QJsonDocument>
#include <QProcess>
#include <QInputDialog>

SaveSettings::SaveSettings(LXQt::Settings*applicationSettings, QWidget* parent):
    QDialog(parent)
{

    this->applicationSettings = applicationSettings;

    ui.setupUi(this);

    QSize size(128,64);
    ui.save->setIcon(QIcon::fromTheme("document-save"));
    ui.save->setIconSize(size);

    connect(ui.hardwareCompatibleConfigs, SIGNAL(itemDoubleClicked(QListWidgetItem *)), SLOT(setSavedSettings(QListWidgetItem *)));
    connect(ui.deletePushButton, SIGNAL(clicked()), SLOT(onDeleteItem()));
    connect(ui.renamePushButton, SIGNAL(clicked()), SLOT(onRenameItem()));

    loadSettings();
}

void SaveSettings::setHardwareIdentifier(QString hardwareIdentifier)
{
    this->hardwareIdentifier = hardwareIdentifier;
    loadSettings();
}

void SaveSettings::setSavedSettings(QListWidgetItem * item)
{
    QJsonObject o = item->data(Qt::UserRole).toJsonObject();
    QString cmd = o["command"].toString();
    qDebug() << "[SaveSettings::setSavedSettings]: " << cmd;
    QProcess::execute(cmd);
}

void SaveSettings::onDeleteItem()
{
    if( ui.allConfigs->currentItem() == NULL )
        return;
    QJsonObject obj  = ui.allConfigs->currentItem()->data(Qt::UserRole).toJsonObject();
    applicationSettings->beginGroup("configMonitor");
    QJsonArray  savedConfigs = QJsonDocument::fromJson(applicationSettings->value("saved").toByteArray()).array();
    for(int i=0; i<savedConfigs.size(); i++) {
        const QJsonValue & v = savedConfigs[i];
        QJsonObject o = v.toObject();
        if( o["name"].toString() == obj["name"].toString() ) {
            savedConfigs.removeAt(i);
            break;
        }
    }
    applicationSettings->setValue("saved", QVariant(QJsonDocument(savedConfigs).toJson()));
    applicationSettings->endGroup();
    loadSettings();
}

void SaveSettings::onRenameItem()
{
    if( ui.allConfigs->currentItem() == NULL )
        return;
    QJsonObject obj  = ui.allConfigs->currentItem()->data(Qt::UserRole).toJsonObject();
    bool ok;
    QString configName = QInputDialog::getText(this, tr("Name"), tr("Name:"), QLineEdit::Normal, obj["name"].toString(), &ok);
    if (!ok || configName.isEmpty())
        return;
    applicationSettings->beginGroup("configMonitor");
    QJsonArray  savedConfigs = QJsonDocument::fromJson(applicationSettings->value("saved").toByteArray()).array();
    for(int i=0; i<savedConfigs.size(); i++) {
        const QJsonValue & v = savedConfigs[i];
        QJsonObject o = v.toObject();
        if( o["name"].toString() == obj["name"].toString() ) {
            savedConfigs.removeAt(i);
            obj["name"] = configName;
            savedConfigs.append(obj);
            break;
        }
    }
    applicationSettings->setValue("saved", QVariant(QJsonDocument(savedConfigs).toJson()));
    applicationSettings->endGroup();
    loadSettings();
}

void SaveSettings::loadSettings()
{
    ui.allConfigs->clear();
    ui.hardwareCompatibleConfigs->clear();
    applicationSettings->beginGroup("configMonitor");
    QJsonArray  savedConfigs = QJsonDocument::fromJson(applicationSettings->value("saved").toByteArray()).array();
    foreach (const QJsonValue & v, savedConfigs) {
        QJsonObject o = v.toObject();
        QListWidgetItem *item = new QListWidgetItem(o["name"].toString(), ui.allConfigs);
        item->setData(Qt::UserRole, QVariant(o));
        if(o["hardwareIdentifier"].toString() == hardwareIdentifier) {
            QListWidgetItem *item = new QListWidgetItem(o["name"].toString(), ui.hardwareCompatibleConfigs);
            item->setData(Qt::UserRole, QVariant(o));
        }
    }
    applicationSettings->endGroup();
}
