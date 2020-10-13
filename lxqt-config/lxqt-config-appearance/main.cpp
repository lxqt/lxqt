/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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

#include <LXQt/SingleApplication>

#include <LXQt/Settings>
#include <LXQt/ConfigDialog>
#include <QCommandLineParser>
#include "iconthemeconfig.h"
#include "lxqtthemeconfig.h"
#include "styleconfig.h"
#include "fontsconfig.h"
#include "configothertoolkits.h"

#include "../liblxqt-config-cursor/selectwnd.h"

int main (int argc, char **argv)
{
    LXQt::SingleApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LXQt Config Appearance"));
    const QString VERINFO = QStringLiteral(LXQT_CONFIG_VERSION
                                           "\nliblxqt   " LXQT_VERSION
                                           "\nQt        " QT_VERSION_STR);
    app.setApplicationVersion(VERINFO);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    LXQt::Settings* settings = new LXQt::Settings(QStringLiteral("lxqt"));
    LXQt::Settings* sessionSettings = new LXQt::Settings(QStringLiteral("session"));
    LXQt::ConfigDialog* dialog = new LXQt::ConfigDialog(QObject::tr("LXQt Appearance Configuration"), settings);
    dialog->setButtons(QDialogButtonBox::Apply|QDialogButtonBox::Close|QDialogButtonBox::Reset);
    dialog->enableButton(QDialogButtonBox::Apply, false); // disable Apply button in the beginning

    app.setActivationWindow(dialog);

    LXQt::Settings mConfigAppearanceSettings(QStringLiteral("lxqt-config-appearance"));
    ConfigOtherToolKits *configOtherToolKits = new ConfigOtherToolKits(settings, &mConfigAppearanceSettings, dialog);

    QSettings& qtSettings = *settings; // use lxqt config file for Qt settings in Qt5.

    /*** Widget Style ***/
    StyleConfig* stylePage = new StyleConfig(settings, &qtSettings, &mConfigAppearanceSettings, configOtherToolKits, dialog);
    dialog->addPage(stylePage, QObject::tr("Widget Style"), QStringList() << QStringLiteral("preferences-desktop-theme") << QStringLiteral("preferences-desktop"));
    QObject::connect(dialog, &LXQt::ConfigDialog::reset, stylePage, &StyleConfig::initControls);
    QObject::connect(stylePage, &StyleConfig::settingsChanged, dialog, [dialog] {
        dialog->enableButton(QDialogButtonBox::Apply, true); // enable Apply button when something is changed
    });

    /*** Icon Theme ***/
    IconThemeConfig* iconPage = new IconThemeConfig(settings, dialog);
    dialog->addPage(iconPage, QObject::tr("Icons Theme"), QStringList() << QStringLiteral("preferences-desktop-icons") << QStringLiteral("preferences-desktop"));
    QObject::connect(dialog, &LXQt::ConfigDialog::reset, iconPage, &IconThemeConfig::initControls);
    QObject::connect(iconPage, &IconThemeConfig::settingsChanged, dialog, [dialog] {
        dialog->enableButton(QDialogButtonBox::Apply, true);
    });
    QObject::connect(iconPage, &IconThemeConfig::updateOtherSettings, configOtherToolKits, &ConfigOtherToolKits::setConfig);

    /*** LXQt Theme ***/
    LXQtThemeConfig* themePage = new LXQtThemeConfig(settings, dialog);
    dialog->addPage(themePage, QObject::tr("LXQt Theme"), QStringList() << QStringLiteral("preferences-desktop-color") << QStringLiteral("preferences-desktop"));
    QObject::connect(dialog, &LXQt::ConfigDialog::reset, themePage, &LXQtThemeConfig::initControls);
    QObject::connect(themePage, &LXQtThemeConfig::settingsChanged, dialog, [dialog] {
        dialog->enableButton(QDialogButtonBox::Apply, true);
    });

    /*** Font ***/
    FontsConfig* fontsPage = new FontsConfig(settings, &qtSettings, dialog);
    dialog->addPage(fontsPage, QObject::tr("Font"), QStringList() << QStringLiteral("preferences-desktop-font") << QStringLiteral("preferences-desktop"));
    QObject::connect(dialog, &LXQt::ConfigDialog::reset, fontsPage, &FontsConfig::initControls);
    QObject::connect(fontsPage, &FontsConfig::updateOtherSettings, configOtherToolKits, &ConfigOtherToolKits::setConfig);
    QObject::connect(fontsPage, &FontsConfig::settingsChanged, dialog, [dialog] {
        dialog->enableButton(QDialogButtonBox::Apply, true);
    });

    /*** Cursor Theme ***/
    SelectWnd* cursorPage = new SelectWnd(sessionSettings, dialog);
    cursorPage->setCurrent();
    dialog->addPage(cursorPage, QObject::tr("Cursor"), QStringList() << QStringLiteral("input-mouse") << QStringLiteral("preferences-desktop"));
    QObject::connect(cursorPage, &SelectWnd::settingsChanged, dialog, [dialog] {
        dialog->enableButton(QDialogButtonBox::Apply, true);
    });

    // apply all changes on clicking Apply
    QObject::connect(dialog, &LXQt::ConfigDialog::clicked, [=] (QDialogButtonBox::StandardButton btn) {
        if (btn == QDialogButtonBox::Apply)
        {
            iconPage->applyIconTheme();
            themePage->applyLxqtTheme();
            fontsPage->updateQtFont();
            cursorPage->applyCusorTheme();
            stylePage->applyStyle(); // Cursor and font have to be set before style
            // disable Apply button after changes are applied
            dialog->enableButton(btn, false);
        }
        else if (btn == QDialogButtonBox::Reset)
            dialog->enableButton(QDialogButtonBox::Apply, false); // disable Apply button on resetting too
    });

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-theme")));
    dialog->show();

    return app.exec();
}

