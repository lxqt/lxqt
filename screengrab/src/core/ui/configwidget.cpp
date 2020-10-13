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

#include <QKeyEvent>
#include "configwidget.h"
#include "ui_configwidget.h"
#include "../core.h"
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QPushButton>
ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::configwidget)
{
    _ui->setupUi(this);
    _ui->buttonBox_1->button(QDialogButtonBox::RestoreDefaults)->setText(tr("Restore Defaults"));
    _ui->buttonBox_2->button(QDialogButtonBox::Save)->setText(tr("Save"));
    _ui->buttonBox_3->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    conf = Config::instance();
    connect(_ui->buttonBox_2->button(QDialogButtonBox::Save), &QPushButton::clicked, this, &ConfigDialog::saveSettings);
    connect(_ui->buttonBrowse, &QPushButton::clicked, this, &ConfigDialog::selectDir);
    connect(_ui->buttonBox_1->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &ConfigDialog::restoreDefaults);
    connect(_ui->checkIncDate, &QCheckBox::toggled, this, &ConfigDialog::setVisibleDateTplEdit);
    connect(_ui->keyWidget, &QKeySequenceWidget::keySequenceAccepted, this, &ConfigDialog::acceptShortcut);
    connect(_ui->keyWidget, &QKeySequenceWidget::keyNotSupported, this, &ConfigDialog::keyNotSupported);
    connect(_ui->checkAutoSave, &QCheckBox::toggled, this, &ConfigDialog::setVisibleAutoSaveFirst);
    connect(_ui->buttonBox_3->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &ConfigDialog::reject);
    connect(_ui->treeKeys, &QTreeWidget::expanded, _ui->treeKeys, &QTreeWidget::clearSelection);
    connect(_ui->treeKeys, &QTreeWidget::collapsed, this, &ConfigDialog::collapsTreeKeys);
    connect(_ui->checkShowTray, &QCheckBox::toggled, this, &ConfigDialog::toggleCheckShowTray);
    connect(_ui->editDateTmeTpl, &QLineEdit::textEdited, this, &ConfigDialog::editDateTmeTpl);

    void (QSpinBox::*timeToTray)(int) = &QSpinBox::valueChanged;
    connect(_ui->timeTrayMess, timeToTray, this, &ConfigDialog::changeTimeTrayMess);

    void (QComboBox::*trayMessType)(int) = &QComboBox::currentIndexChanged;
    connect(_ui->cbxTrayMsg, trayMessType, this, &ConfigDialog::changeTrayMsgType);

    connect(_ui->treeKeys, &QTreeWidget::doubleClicked, this, &ConfigDialog::doubleclickTreeKeys);
    connect(_ui->treeKeys, &QTreeWidget::activated, this, &ConfigDialog::doubleclickTreeKeys);
    connect(_ui->treeKeys->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ConfigDialog::currentItemChanged);
    connect(_ui->keyWidget, &QKeySequenceWidget::keySequenceCleared, this, &ConfigDialog::clearShrtcut);
    connect(_ui->listWidget, &QListWidget::currentRowChanged, _ui->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(_ui->slideImgQuality, &QSlider::valueChanged, this, &ConfigDialog::changeImgQualituSlider);

    void (QComboBox::*formatChabge)(int) = &QComboBox::currentIndexChanged;
    connect(_ui->cbxFormat, formatChabge, this, &ConfigDialog::changeFormatType);

    loadSettings();
    setVisibleDateTplEdit(conf->getDateTimeInFilename());

    setVisibleAutoSaveFirst(conf->getAutoSave());

    _ui->listWidget->setCurrentRow(0);

    editDateTmeTpl(conf->getDateTimeTpl());

    _ui->treeKeys->expandAll();
    _ui->treeKeys->header()->setSectionResizeMode(QHeaderView::Stretch);

    // adding shortcut values in treewidge
    int action = 0;
    QTreeWidgetItemIterator iter(_ui->treeKeys);
    while (*iter)
    {
        if ((*iter)->parent() != nullptr)
        {
            (*iter)->setData(1, Qt::DisplayRole, conf->shortcuts()->getShortcut(action));
            ++action;
        }
        ++iter;
    }

    // set false visibility to edit hokey controls
    _ui->labUsedShortcut->setVisible(false);
    _ui->keyWidget->setVisible(false);

    // Load config widgets for modules
    quint8 countModules = Core::instance()->modules()->count();

    for (int i = 0; i < countModules; ++i)
    {
        AbstractModule* currentModule = Core::instance()->modules()->getModule(i);

        if (currentModule->initConfigWidget() != nullptr)
        {
            _ui->listWidget->addItem(currentModule->moduleName());
            QWidget *currentModWidget = currentModule->initConfigWidget();
            _ui->stackedWidget->addWidget(currentModWidget);
            _moduleWidgetNames << currentModWidget->objectName();
        }
    }
}

ConfigDialog::~ConfigDialog()
{
    delete _ui;
    conf = nullptr;
    delete conf;
}

void ConfigDialog::loadSettings()
{
    // main tab
    _ui->editDir->setText(conf->getSaveDir());
    _ui->editFileName->setText(conf->getSaveFileName());

    _ui->cbxFormat->addItems(conf->getFormatIDs());
    _ui->cbxFormat->setCurrentIndex(conf->getDefaultFormatID());

    _ui->checkIncDate->setChecked(conf->getDateTimeInFilename());
    _ui->editDateTmeTpl->setText(conf->getDateTimeTpl());
    _ui->cbxCopyFileName->setCurrentIndex(conf->getAutoCopyFilenameOnSaving());

    // display tab
    _ui->cbxTrayMsg->setCurrentIndex(conf->getTrayMessages());
    changeTrayMsgType(_ui->cbxTrayMsg->currentIndex());
    _ui->timeTrayMess->setValue(conf->getTimeTrayMess());
    _ui->checkAutoSave->setChecked(conf->getAutoSave());
    _ui->checkAutoSaveFirst->setChecked(conf->getAutoSaveFirst());

    _ui->checkInTray->setChecked(conf->getCloseInTray());
    _ui->checkAllowCopies->setChecked(conf->getAllowMultipleInstance());

    _ui->checkShowTray->setChecked(conf->getShowTrayIcon());
    toggleCheckShowTray(conf->getShowTrayIcon());

    _ui->slideImgQuality->setValue(conf->getImageQuality());
    _ui->cbxEnableExtView->setChecked(conf->getEnableExtView());

    _ui->checkFitInside->setChecked(conf->getFitInside());
}


void ConfigDialog::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        _ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ConfigDialog::setVisibleAutoSaveFirst(bool status)
{
    _ui->checkAutoSaveFirst->setVisible(status);
}

void ConfigDialog::changeFormatType(int type)
{
    if (type == 1)
        _ui->groupQuality->setVisible(true);
    else
        _ui->groupQuality->setVisible(false);
}


void ConfigDialog::changeImgQualituSlider(int pos)
{
    QString text = QLatin1Char(' ') + QString::number(pos) + QLatin1Char('%');
    _ui->labImgQualityCurrent->setText(text);
}

void ConfigDialog::saveSettings()
{
    QDir screenshotDir(_ui->editDir->text());
    if (!screenshotDir.exists())
    {
        QMessageBox msg;
        msg.setText(tr("Directory %1 does not exist. Do you want to create it?").arg(QDir::toNativeSeparators(screenshotDir.path()) + QDir::separator()));
        msg.setWindowTitle(QStringLiteral("ScreenGrab") + QStringLiteral(" - ") + tr("Warning"));
        msg.setIcon(QMessageBox::Question);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        int res = msg.exec();

        if (res == QMessageBox::No)
            return;
        else
        {
            screenshotDir.mkpath(screenshotDir.path());

            if (!screenshotDir.path().endsWith(QDir::separator()))
            {
                QString updatedPath = screenshotDir.path() + QDir::separator();
                updatedPath = QDir::toNativeSeparators(updatedPath);
                _ui->editDir->setText(updatedPath);
            }
        }
    }

    // set new values of general settings
    conf->setSaveDir(_ui->editDir->text());
    conf->setSaveFileName(_ui->editFileName->text());
    conf->setSaveFormat(_ui->cbxFormat->currentText());
    conf->setDateTimeInFilename(_ui->checkIncDate->isChecked());
    conf->setDateTimeTpl(_ui->editDateTmeTpl->text());
    conf->setAutoCopyFilenameOnSaving(_ui->cbxCopyFileName->currentIndex());
    conf->setAutoSave(_ui->checkAutoSave->isChecked());
    conf->setAutoSaveFirst(_ui->checkAutoSaveFirst->isChecked());
    conf->setTrayMessages(_ui->cbxTrayMsg->currentIndex());
    conf->setCloseInTray(_ui->checkInTray->isChecked());
    conf->setAllowMultipleInstance(_ui->checkAllowCopies->isChecked());
    conf->setTimeTrayMess(_ui->timeTrayMess->value());
    conf->setShowTrayIcon(_ui->checkShowTray->isChecked());
    conf->setImageQuality(_ui->slideImgQuality->value());
    conf->setEnableExtView(_ui->cbxEnableExtView->isChecked());
    conf->setFitInside(_ui->checkFitInside->isChecked());

    // save shortcuts in shortcutmanager
    int action = 0;
    QTreeWidgetItemIterator iter(_ui->treeKeys);
    while (*iter)
    {
        if ((*iter)->parent())
        {
            switch((*iter)->parent()->childCount())
            {
            case 3:
                conf->shortcuts()->setShortcut((*iter)->data(1, Qt::DisplayRole).toString(), action, 0);
                break;
            case 6:
                conf->shortcuts()->setShortcut((*iter)->data(1, Qt::DisplayRole).toString(), action, 1);
                break;
            default:
                break;
            }
            ++action;
        }
        ++iter;
    }

    // update values of front-end settings
    conf->saveSettings();

    // call save method on modeule's configwidgets'
    for (int i = 0; i < _moduleWidgetNames.count(); ++i)
    {
        QString name = _moduleWidgetNames.at(i);
        QWidget* currentWidget = _ui->stackedWidget->findChild<QWidget*>(name);
        if (currentWidget)
            QMetaObject::invokeMethod(currentWidget, "saveSettings");
    }

    accept();
}

QString ConfigDialog::getFormat()
{
    switch (_ui->cbxFormat->currentIndex())
    {
    case 0: return QStringLiteral("png");
    case 1: return QStringLiteral("jpg");
    default: return QStringLiteral("png");
    }
}

void ConfigDialog::selectDir()
{
    QString directory = QFileDialog::getExistingDirectory(this, trUtf8("Select directory"),
                                                          _ui->editDir->text(), QFileDialog::ShowDirsOnly);

    if (!directory.isEmpty()) {
        _ui->editDir->setText(QDir::toNativeSeparators(directory));
    }
}

void ConfigDialog::restoreDefaults()
{
    QMessageBox msg;
    msg.setText(tr("Do you want to reset the settings to the defaults?"));
    msg.setWindowTitle(QStringLiteral("ScreenGrab") + QStringLiteral(" - ") + tr("Warning"));
    msg.setIcon(QMessageBox::Question);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    int res = msg.exec();

    if (res == QMessageBox::Yes)
    {
        conf->setDefaultSettings();
        conf->saveSettings();
        QDialog::accept();
    }
}

void ConfigDialog::changeTimeTrayMess(int sec)
{
    conf->setTimeTrayMess(sec);
}

void ConfigDialog::changeTrayMsgType(int type)
{
    switch(type)
    {
    case 0:
    {
        _ui->labTimeTrayMess->setVisible(false);
        _ui->timeTrayMess->setVisible(false);
        break;
    }
    default:
    {
        _ui->labTimeTrayMess->setVisible(true);
        _ui->timeTrayMess->setVisible(true);
        break;
    }
    }
}

void ConfigDialog::setVisibleDateTplEdit(bool checked)
{
    if (!checked)
    {
        _ui->editDateTmeTpl->setVisible(false);
        _ui->labMask->setVisible(false);
        _ui->labMaskExample->setVisible(false);
    }
    else
    {
        _ui->editDateTmeTpl->setVisible(true);
        _ui->labMask->setVisible(true);
        _ui->labMaskExample->setVisible(true);
    }
}

void ConfigDialog::editDateTmeTpl(const QString &str)
{
    QString currentDateTime = QDateTime::currentDateTime().toString(str);
    _ui->labMaskExample->setText(tr("Example: ") + currentDateTime);
}

void ConfigDialog::toggleCheckShowTray(bool checked)
{
    _ui->labTrayMessages->setEnabled(checked);
    _ui->cbxTrayMsg->setEnabled(checked);
    _ui->timeTrayMess->setEnabled(checked);
    _ui->labTimeTrayMess->setEnabled(checked);
    _ui->checkInTray->setEnabled(checked);
}

void ConfigDialog::currentItemChanged(const QModelIndex c, const QModelIndex p)
{
    Q_UNUSED(p)
    if (c.parent().isValid())
    {
        _ui->labUsedShortcut->setVisible(true);
        _ui->keyWidget->setVisible(true);

        QTreeWidgetItem *item = _ui->treeKeys->currentItem();
        _ui->keyWidget->setKeySequence(QKeySequence(item->data(1, Qt::DisplayRole).toString()));
    }
    else
    {
        _ui->labUsedShortcut->setVisible(false);
        _ui->keyWidget->setVisible(false);
    }
}

void ConfigDialog::doubleclickTreeKeys(QModelIndex index)
{
    if (index.parent().isValid())
    {
        connect(_ui->keyWidget, SIGNAL(keySequenceChanged(QKeySequence)), this, SLOT(changeShortcut(QKeySequence)));
        _ui->keyWidget->captureKeySequence();
    }
}

void ConfigDialog::collapsTreeKeys(QModelIndex index)
{
    if (!index.parent().isValid())
    {
        _ui->labUsedShortcut->setVisible(false);
        _ui->keyWidget->setVisible(false);
    }
}

void ConfigDialog::acceptShortcut(const QKeySequence& seq)
{
    if (!checkUsedShortcuts())
    {
        changeShortcut(seq);
    }
    else if (checkUsedShortcuts() && !seq.toString().isEmpty())
        showErrorMessage(tr("This key is already used in ScreenGrab! Please select another."));
}

void ConfigDialog::changeShortcut(const QKeySequence& seq)
{
    disconnect(_ui->keyWidget, SIGNAL(keySequenceChanged(QKeySequence)), this, SLOT(changeShortcut(QKeySequence)));
    QTreeWidgetItem *item = _ui->treeKeys->selectedItems().first();
    item->setData(1, Qt::DisplayRole, seq.toString());
}

void ConfigDialog::clearShrtcut()
{
    QTreeWidgetItem *item = _ui->treeKeys->selectedItems().first();
    item->setData(1, Qt::DisplayRole, QLatin1String(""));
}

void ConfigDialog::keyNotSupported()
{
    showErrorMessage(tr("This key is not supported on your system!"));
}

bool ConfigDialog::checkUsedShortcuts()
{
    QTreeWidgetItem *item = _ui->treeKeys->selectedItems().first();
    QTreeWidgetItemIterator iter(_ui->treeKeys);
    while (*iter)
    {
        if ((*iter) != item && (*iter)->data(1, Qt::DisplayRole) == _ui->keyWidget->keySequence().toString())
            return true;
        ++iter;
    }
    return false;
}

void ConfigDialog::showErrorMessage(const QString &text)
{
    _ui->keyWidget->clearKeySequence();
    QMessageBox msg;
    msg.setWindowTitle(tr("Error"));
    msg.setText(text);
    msg.setIcon(QMessageBox::Information);
    msg.setStandardButtons(QMessageBox::Ok);
    msg.exec();
}
