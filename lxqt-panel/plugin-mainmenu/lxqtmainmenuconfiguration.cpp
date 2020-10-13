/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
 *   Maciej Płaza <plaza.maciej@gmail.com>
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


#include "lxqtmainmenuconfiguration.h"
#include "ui_lxqtmainmenuconfiguration.h"
#include <XdgMenu>
#include <XdgIcon>
#include <lxqt-globalkeys.h>
#include <LXQt/Settings>
#include <QPushButton>
#include <QAction>
#include <QFileDialog>

LXQtMainMenuConfiguration::LXQtMainMenuConfiguration(PluginSettings *settings, GlobalKeyShortcut::Action * shortcut, const QString &defaultShortcut, QWidget *parent) :
    LXQtPanelPluginConfigDialog(settings, parent),
    ui(new Ui::LXQtMainMenuConfiguration),
    mDefaultShortcut(defaultShortcut),
    mShortcut(shortcut)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName(QStringLiteral("MainMenuConfigurationWindow"));
    ui->setupUi(this);
    ui->buttonBox_1->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
    ui->buttonBox_2->button(QDialogButtonBox::Close)->setText(tr("Close"));
    QIcon folder{XdgIcon::fromTheme(QStringLiteral("folder"))};
    ui->chooseMenuFilePB->setIcon(folder);
    ui->iconPB->setIcon(folder);
    connect(ui->buttonBox_1, &QDialogButtonBox::clicked, this, &LXQtMainMenuConfiguration::dialogButtonsAction);

    loadSettings();

    connect(ui->showTextCB, &QAbstractButton::toggled, this, &LXQtMainMenuConfiguration::showTextChanged);
    connect(ui->textLE, &QLineEdit::textEdited, this, &LXQtMainMenuConfiguration::textButtonChanged);
    connect(ui->chooseMenuFilePB, &QAbstractButton::clicked, this, &LXQtMainMenuConfiguration::chooseMenuFile);
    connect(ui->menuFilePathLE, &QLineEdit::textChanged, [&] (QString const & file)
        {
            this->settings().setValue(QLatin1String("menu_file"), file);
        });
    connect(ui->iconCB, &QCheckBox::toggled, [this] (bool value) { this->settings().setValue(QStringLiteral("ownIcon"), value); });
    connect(ui->iconPB, &QAbstractButton::clicked, this, &LXQtMainMenuConfiguration::chooseIcon);
    connect(ui->iconLE, &QLineEdit::textChanged, [&] (QString const & path)
        {
            this->settings().setValue(QLatin1String("icon"), path);
        });

    connect(ui->shortcutEd, &ShortcutSelector::shortcutGrabbed, this, &LXQtMainMenuConfiguration::shortcutChanged);
    connect(ui->shortcutEd->addMenuAction(tr("Reset")), &QAction::triggered, this, &LXQtMainMenuConfiguration::shortcutReset);

    connect(ui->customFontCB, &QAbstractButton::toggled, this, &LXQtMainMenuConfiguration::customFontChanged);
    connect(ui->customFontSizeSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &LXQtMainMenuConfiguration::customFontSizeChanged);

    connect(mShortcut, &GlobalKeyShortcut::Action::shortcutChanged, this, &LXQtMainMenuConfiguration::globalShortcutChanged);

    connect(ui->filterMenuCB, &QCheckBox::toggled, [this] (bool enabled)
        {
            ui->filterClearCB->setEnabled(enabled || ui->filterShowCB->isChecked());
            this->settings().setValue(QStringLiteral("filterMenu"), enabled);
        });
    connect(ui->filterShowCB, &QCheckBox::toggled, [this] (bool enabled)
        {
            ui->filterClearCB->setEnabled(enabled || ui->filterMenuCB->isChecked());
            this->settings().setValue(QStringLiteral("filterShow"), enabled);
        });
    connect(ui->filterShowMaxItemsSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this] (int value)
        {
            this->settings().setValue(QStringLiteral("filterShowMaxItems"), value);
        });
    connect(ui->filterShowMaxWidthSB, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this] (int value)
        {
            this->settings().setValue(QStringLiteral("filterShowMaxWidth"), value);
        });
    connect(ui->filterShowHideMenuCB, &QCheckBox::toggled, [this] (bool enabled)
        {
            this->settings().setValue(QStringLiteral("filterShowHideMenu"), enabled);
        });
    connect(ui->filterClearCB, &QCheckBox::toggled, [this] (bool enabled)
        {
            this->settings().setValue(QStringLiteral("filterClear"), enabled);
        });
}

LXQtMainMenuConfiguration::~LXQtMainMenuConfiguration()
{
    delete ui;
}

void LXQtMainMenuConfiguration::loadSettings()
{
    ui->iconCB->setChecked(settings().value(QStringLiteral("ownIcon"), false).toBool());
    ui->iconLE->setText(settings().value(QStringLiteral("icon"), QLatin1String(LXQT_GRAPHICS_DIR"/helix.svg")).toString());
    ui->showTextCB->setChecked(settings().value(QStringLiteral("showText"), false).toBool());
    ui->textLE->setText(settings().value(QStringLiteral("text"), QString()).toString());

    QString menuFile = settings().value(QStringLiteral("menu_file"), QString()).toString();
    if (menuFile.isEmpty())
    {
        menuFile = XdgMenu::getMenuFileName();
    }
    ui->menuFilePathLE->setText(menuFile);
    ui->shortcutEd->setText(nullptr != mShortcut ? mShortcut->shortcut() : mDefaultShortcut);

    ui->customFontCB->setChecked(settings().value(QStringLiteral("customFont"), false).toBool());
    LXQt::Settings lxqtSettings(QStringLiteral("lxqt")); //load system font size as init value
    QFont systemFont;
    lxqtSettings.beginGroup(QLatin1String("Qt"));
    systemFont.fromString(lxqtSettings.value(QStringLiteral("font"), this->font()).toString());
    lxqtSettings.endGroup();
    ui->customFontSizeSB->setValue(settings().value(QStringLiteral("customFontSize"), systemFont.pointSize()).toInt());
    const bool filter_menu = settings().value(QStringLiteral("filterMenu"), true).toBool();
    ui->filterMenuCB->setChecked(filter_menu);
    const bool filter_show = settings().value(QStringLiteral("filterShow"), true).toBool();
    ui->filterShowCB->setChecked(filter_show);
    ui->filterShowMaxItemsL->setEnabled(filter_show);
    ui->filterShowMaxItemsSB->setEnabled(filter_show);
    ui->filterShowMaxItemsSB->setValue(settings().value(QStringLiteral("filterShowMaxItems"), 10).toInt());
    ui->filterShowMaxWidthL->setEnabled(filter_show);
    ui->filterShowMaxWidthSB->setEnabled(filter_show);
    ui->filterShowMaxWidthSB->setValue(settings().value(QStringLiteral("filterShowMaxWidth"), 300).toInt());
    ui->filterShowHideMenuCB->setEnabled(filter_show);
    ui->filterShowHideMenuCB->setChecked(settings().value(QStringLiteral("filterShowHideMenu"), true).toBool());
    ui->filterClearCB->setChecked(settings().value(QStringLiteral("filterClear"), false).toBool());
    ui->filterClearCB->setEnabled(filter_menu || filter_show);
}


void LXQtMainMenuConfiguration::textButtonChanged(const QString &value)
{
    settings().setValue(QStringLiteral("text"), value);
}

void LXQtMainMenuConfiguration::showTextChanged(bool value)
{
    settings().setValue(QStringLiteral("showText"), value);
}

void LXQtMainMenuConfiguration::chooseIcon()
{
    QFileInfo f{ui->iconLE->text()};
    QDir dir = f.dir();
    QFileDialog *d = new QFileDialog(this,
                                     tr("Choose icon file"),
                                     !f.filePath().isEmpty() && dir.exists() ? dir.path() : QLatin1String(LXQT_GRAPHICS_DIR),
                                     tr("Images (*.svg *.png)"));
    d->setWindowModality(Qt::WindowModal);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &QFileDialog::fileSelected, [&] (const QString &icon) {
        ui->iconLE->setText(icon);
    });
    d->show();
}

void LXQtMainMenuConfiguration::chooseMenuFile()
{
    QFileDialog *d = new QFileDialog(this,
                                     tr("Choose menu file"),
                                     QLatin1String("/etc/xdg/menus"),
                                     tr("Menu files (*.menu)"));
    d->setWindowModality(Qt::WindowModal);
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &QFileDialog::fileSelected, [&] (const QString &file) {
        ui->menuFilePathLE->setText(file);
    });
    d->show();
}

void LXQtMainMenuConfiguration::globalShortcutChanged(const QString &/*oldShortcut*/, const QString &newShortcut)
{
    ui->shortcutEd->setText(newShortcut);
}

void LXQtMainMenuConfiguration::shortcutChanged(const QString &value)
{
    if (mShortcut)
        mShortcut->changeShortcut(value);
}

void LXQtMainMenuConfiguration::shortcutReset()
{
    shortcutChanged(mDefaultShortcut);
}

void LXQtMainMenuConfiguration::customFontChanged(bool value)
{
    settings().setValue(QStringLiteral("customFont"), value);
}

void LXQtMainMenuConfiguration::customFontSizeChanged(int value)
{
    settings().setValue(QStringLiteral("customFontSize"), value);
}
