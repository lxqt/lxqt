/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)GPL2+
 *
 *
 * Copyright: 2014 LXQt team
 *
 * Authors:
 *   Julien Lavergne <gilir@ubuntu.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include <LXQt/SingleApplication>

#include <LXQt/Settings>
#include <LXQt/ConfigDialog>
#include <QCommandLineParser>
#include "localeconfig.h"

int main (int argc, char **argv)
{
    LXQt::SingleApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("LXQt Config Locale"));
    const QString VERINFO = QStringLiteral(LXQT_CONFIG_VERSION
                                           "\nliblxqt   " LXQT_VERSION
                                           "\nQt        " QT_VERSION_STR);
    app.setApplicationVersion(VERINFO);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(app);

    LXQt::Settings settings(QStringLiteral("lxqt-config-locale"));
    LXQt::Settings session_settings(QStringLiteral("session"));
    LXQt::ConfigDialog* dialog = new LXQt::ConfigDialog(QObject::tr("LXQt Locale Configuration"), &settings);

    app.setActivationWindow(dialog);

    LocaleConfig* localePage = new LocaleConfig(&settings, &session_settings, dialog);
    dialog->addPage(localePage, QObject::tr("Locale Settings"), QStringList() << QStringLiteral("preferences-desktop-locale") << QStringLiteral("preferences-desktop"));
    QObject::connect(dialog, SIGNAL(reset()), localePage, SLOT(initControls()));
    QObject::connect(dialog, SIGNAL(save()), localePage, SLOT(saveSettings()));

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop-locale")));
    dialog->show();

    return app.exec();
}
