/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2014 LXQt team
 * Authors:
 *   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#include "styleconfig.h"
#include "ui_styleconfig.h"
#include <QTreeWidget>
#include <QDebug>
#include <QStyleFactory>
#include <QToolBar>
#include <QSettings>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaEnum>
#include <QToolBar>

#ifdef Q_WS_X11
extern void qt_x11_apply_settings_in_all_apps();
#endif

StyleConfig::StyleConfig(LXQt::Settings* settings, QSettings* qtSettings, LXQt::Settings *configAppearanceSettings, ConfigOtherToolKits *configOtherToolKits, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::StyleConfig),
    mQtSettings(qtSettings),
    mSettings(settings)
{
    mConfigAppearanceSettings = configAppearanceSettings;
    mConfigOtherToolKits = configOtherToolKits;
    ui->setupUi(this);

    initControls();

    connect(ui->advancedOptionsGroupBox, &QGroupBox::toggled, this, &StyleConfig::showAdvancedOptions);

    connect(ui->qtComboBox, QOverload<int>::of(&QComboBox::activated), this, &StyleConfig::settingsChanged);
    connect(ui->advancedOptionsGroupBox, &QGroupBox::clicked, this, &StyleConfig::settingsChanged);
    connect(ui->gtk2ComboBox, QOverload<int>::of(&QComboBox::activated), this, &StyleConfig::settingsChanged);
    connect(ui->gtk3ComboBox, QOverload<int>::of(&QComboBox::activated), this, &StyleConfig::settingsChanged);
    connect(ui->toolButtonStyle, QOverload<int>::of(&QComboBox::activated), this, &StyleConfig::settingsChanged);
    connect(ui->singleClickActivate, &QAbstractButton::clicked, this, &StyleConfig::settingsChanged);
    connect(ui->winColorLabel, &ColorLabel::colorChanged, this, &StyleConfig::settingsChanged);
}


StyleConfig::~StyleConfig()
{
    delete ui;
}


void StyleConfig::initControls()
{

    // Fill global themes
    QStringList qtThemes = QStyleFactory::keys();
    QStringList gtk2Themes = mConfigOtherToolKits->getGTKThemes(QStringLiteral("2.0"));
    QStringList gtk3Themes = mConfigOtherToolKits->getGTKThemes(QStringLiteral("3.*"));

    if(!mConfigAppearanceSettings->contains(QStringLiteral("ControlGTKThemeEnabled")))
        mConfigAppearanceSettings->setValue(QStringLiteral("ControlGTKThemeEnabled"), false);
    bool controlGTKThemeEnabled = mConfigAppearanceSettings->value(QStringLiteral("ControlGTKThemeEnabled")).toBool();

    showAdvancedOptions(controlGTKThemeEnabled);
    ui->advancedOptionsGroupBox->setChecked(controlGTKThemeEnabled);

    // read other widget related settings from LXQt settings.
    QByteArray tb_style = mSettings->value(QStringLiteral("tool_button_style")).toByteArray();
    // convert toolbar style name to value
    QMetaEnum me = QToolBar::staticMetaObject.property(QToolBar::staticMetaObject.indexOfProperty("toolButtonStyle")).enumerator();
    int val = me.keyToValue(tb_style.constData());
    if(val == -1)
      val = Qt::ToolButtonTextBesideIcon;
    ui->toolButtonStyle->setCurrentIndex(val);

    // activate item views with single click
    ui->singleClickActivate->setChecked( mSettings->value(QStringLiteral("single_click_activate"), false).toBool());

    // Fill Qt themes
    ui->qtComboBox->clear();
    ui->qtComboBox->addItems(qtThemes);

    // Fill GTK themes
    ui->gtk2ComboBox->addItems(gtk2Themes);
    ui->gtk3ComboBox->addItems(gtk3Themes);

    ui->gtk2ComboBox->setCurrentText(mConfigOtherToolKits->getGTKThemeFromRCFile(QStringLiteral("2.0")));
    ui->gtk3ComboBox->setCurrentText(mConfigOtherToolKits->getGTKThemeFromRCFile(QStringLiteral("3.0")));

    mSettings->beginGroup(QLatin1String("Qt"));
    // Qt style
    ui->qtComboBox->setCurrentText(mSettings->value(QStringLiteral("style")).toString());
    // Qt window color
    QColor color;
    color.setNamedColor(mSettings->value(QStringLiteral("window_color")).toString());
    if (!color.isValid())
        color = QGuiApplication::palette().color(QPalette::Active,QPalette::Window);
    ui->winColorLabel->setColor(color);
    mSettings->endGroup();

    update();
}

void StyleConfig::applyStyle()
{
    // Qt style
    QString themeName = ui->qtComboBox->currentText();;
    mQtSettings->beginGroup(QLatin1String("Qt"));
    if(mQtSettings->value(QStringLiteral("style")).toString() != themeName)
        mQtSettings->setValue(QStringLiteral("style"), themeName);
    // Qt window color
    QColor winColor = ui->winColorLabel->getColor();
    QColor oldWinColor;
    oldWinColor.setNamedColor(mQtSettings->value(QStringLiteral("window_color")).toString());
    if (winColor != oldWinColor)
        mQtSettings->setValue(QStringLiteral("window_color"), winColor.name());
    mQtSettings->endGroup();

    // single click setting
    if(mSettings->value(QStringLiteral("single_click_activate")).toBool() !=  ui->singleClickActivate->isChecked()) {
        mSettings->setValue(QStringLiteral("single_click_activate"), ui->singleClickActivate->isChecked());
    }

   // tool button style
   int index = ui->toolButtonStyle->currentIndex();
    // convert style value to string
    QMetaEnum me = QToolBar::staticMetaObject.property(QToolBar::staticMetaObject.indexOfProperty("toolButtonStyle")).enumerator();
    if(index == -1)
        index = Qt::ToolButtonTextBesideIcon;
    const char* str = me.valueToKey(index);
    if(str && mSettings->value(QStringLiteral("tool_button_style")) != QString::fromUtf8(str))
    {
        mSettings->setValue(QStringLiteral("tool_button_style"), QString::fromUtf8(str));
        mSettings->sync();
        mConfigOtherToolKits->setConfig();
    }

    if (ui->advancedOptionsGroupBox->isChecked())
    {
        // GTK3
        themeName = ui->gtk3ComboBox->currentText();
        mConfigOtherToolKits->setGTKConfig(QStringLiteral("3.0"), themeName);
        // GTK2
        themeName = ui->gtk2ComboBox->currentText();
        mConfigOtherToolKits->setGTKConfig(QStringLiteral("2.0"), themeName);
        // Update xsettingsd
        mConfigOtherToolKits->setXSettingsConfig();
    }
}

void StyleConfig::showAdvancedOptions(bool on)
{
    ui->uniformThemeLabel->setVisible(on);
    mConfigAppearanceSettings->setValue(QStringLiteral("ControlGTKThemeEnabled"), on);
}
